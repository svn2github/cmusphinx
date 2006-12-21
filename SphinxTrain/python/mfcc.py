# mfcc.py: Compute MFCC coefficients (incomplete!)
#
# Copyright (c) 2006 Carnegie Mellon University
#
# You may copy and modify this freely under the same terms as
# Sphinx-III
#
# Author: David Huggins-Daines

from numpy import zeros, arrayrange, dot, shape

def s2dctmat(nfilt,ncep,freqstep):
    """Return the 'legacy' not-quite-DCT matrix used by Sphinx"""
    melcos = zeros((ncep, nfilt), 'double')
    for i in range(0,ncep):
        freq = pi * float(i) / nfilt
        melcos[i] = cos(freq * arrayrange(0.5, float(nfilt)+0.5, 1.0, 'double'))
    melcos[:,0] = melcos[:,0] * 0.5
    return melcos

def logspec2s2mfc(logspec, ncep=13):
    """Convert log-power-spectrum bins to MFCC using the 'legacy'
    Sphinx transform"""
    nframes, nfilt = shape(logspec)
    melcos = s2dctmat(nfilt, ncep, 1/nfilt)
    return dot(logspec, melcos.T) / nfilt

def dctmat(N,K,freqstep):
    """Return the orthogonal DCT-II/DCT-III matrix of size NxK.
    For computing or inverting MFCCs, N is the number of
    log-power-spectrum bins while K is the number of cepstra."""
    cosmat = zeros((N, K), 'double')
    for n in range(0,N):
        for k in range(0, K):
            cosmat[n,k] = cos(freqstep * (n + 0.5) * k)
    cosmat[:,0] = cosmat[:,0] * 1/sqrt(2)
    return cosmat

def dct(input, K=13):
    """Convert log-power-spectrum to MFCC using the orthogonal DCT-II"""
    nframes, N = shape(input)
    freqstep = pi / N
    cosmat = dctmat(N,K,freqstep)
    return dot(input, cosmat) * sqrt(2.0 / N)

def idct(input, K=40):
    """Convert MFCC to log-power-spectrum using the orthogonal DCT-III"""
    nframes, N = shape(input)
    freqstep = pi / K
    cosmat = transpose(dctmat(K,N,freqstep))
    return dot(input, cosmat) * sqrt(2.0 / K)
