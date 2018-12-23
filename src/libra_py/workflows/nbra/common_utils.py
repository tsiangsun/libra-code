#*********************************************************************************
#* Copyright (C) 2017-2018 Brendan A. Smith, Wei Li, Alexey V. Akimov
#*
#* This file is distributed under the terms of the GNU General Public License
#* as published by the Free Software Foundation, either version 2 of
#* the License, or (at your option) any later version.
#* See the file LICENSE in the root directory of this distribution
#* or <http://www.gnu.org/licenses/>.
#*
#*********************************************************************************/
#
#  
#
import sys
import cmath
import math
import os

if sys.platform=="cygwin":
    from cyglibra_core import *
elif sys.platform=="linux" or sys.platform=="linux2":
    from liblibra_core import *
from libra_py import units



def check_input(params, default_params, critical_params):
    """
    params          [dictionary] 
    default_params  [dictionary]
    critical_params [list]

    This function checks whether the required keys are defined in the input.
    If not - print out a worning and set the parameters to the default values provided.

    But the critical parameters should be present - otherwise will exit

    Revises the parameters dictionary - but only according to the subset of 
    default parameters - other ones are not affects
    """

    for key in critical_params:
        if key not in params:
            print "ERROR: The critical parameter ", key, " must be defined!"
            print "Exiting now..."
            sys.exit(0)

    for key in default_params:
        if key not in params:
            print "WARNING: Parameter ", key, " is not defined! in the input parameters"
            print "Use the default value = ", default_params[key]
            params.update({key: default_params[key]})
        else:
            pass
            

    
def get_matrix(nrows, ncols, filename_re, filename_im, act_sp):
    """
    This file reads the real and imaginary components of a matrix of given original size,  
    takes its sub-matrix (as defined by the act_sp function) and returns the resulting 
    complex matrix

    nrows (int)   - the number of rows in the original matrix (read from the files)
    ncols (int)   - the number of columns in the original matrix (read from the files)
    filename_re (string) - the name of the file containing the real part of the matrix 
    filename_im (string) - the name of the file containing the imaginary part of the matrix 
    act_sp (list of ints) - the indices of the columns and rows to be taken to construct the resulting matrices
    
    """

    X_re = MATRIX(nrows, ncols); X_re.Load_Matrix_From_File(filename_re)
    X_im = MATRIX(nrows, ncols); X_im.Load_Matrix_From_File(filename_im)

    nstates = len(act_sp)
    x_re = MATRIX(nstates, nstates);
    x_im = MATRIX(nstates, nstates);

    pop_submatrix(X_re, x_re, act_sp, act_sp)
    pop_submatrix(X_im, x_im, act_sp, act_sp)

    return CMATRIX(x_re, x_im)


def orbs2spinorbs(s):
    """
    This function converts the matrices in the orbital basis (e.g. old PYXAID style)
    to the spin-orbital basis.
    Essentially, it makes a block matrix of a double dimension: 
           ( s  0 )
    s -->  ( 0  s )

    This is meant to be used for backward compatibility with PYXIAD-generated data
    """

    sz = s.num_of_cols
    zero = CMATRIX(sz, sz)    
    act_sp1 = range(0, sz)
    act_sp2 = range(sz, 2*sz)
    
    S = CMATRIX(2*sz, 2*sz)

    push_submatrix(S, s, act_sp1, act_sp1)
    push_submatrix(S, zero, act_sp1, act_sp2)
    push_submatrix(S, zero, act_sp2, act_sp1)
    push_submatrix(S, s, act_sp2, act_sp2)

    return S



def find_maxima(s, logname):
    """
    s [list of double] - data

    This function finds all the maxima of the data set and sorts them according to the data
    The maxima are defined as s[i-1] < s[i] > s[i+1]
   
    Returns the list of the indices of the maximal values
    """

    max_indxs = []
    sz = s.num_of_elems
    for i in xrange(1, sz-1):
        if s.get(i) > s.get(i-1) and s.get(i) > s.get(i+1):
            max_indxs.append(i)

    inp = []
    sz = len(max_indxs)
    for i in xrange(sz):
        inp.append( [ max_indxs[i], s.get(max_indxs[i]) ] )

    out = merge_sort(inp)  # largest in the end

    lgfile = open(logname, "a")
    lgfile.write("Found maxima of the spectrum:\n")
    for i in xrange(sz):
        lgfile.write("index = %3i  frequency index = %8.5f  intensity = %8.5f \n" % (i, out[sz-1-i][0], out[sz-1-i][1]) )
    lgfile.close()    
    
    return out
    

def flt_stat(X):
    """
    Computes the average and std 
    X [list of double] - data
    """
 
    N = len(X)
    res = 0.0

    #===== Average ====
    for i in xrange(N):
        res = res + X[i]
    res = res / float(N)


    #===== Std ========
    res2 = 0.0
    
    for i in xrange(N):
        res2 = res2 + (X[i] - res)**2
    res2 = math.sqrt( res2 / float(N) )

    return res, res2


def mat_stat(X):
    """
    Computes the average and std     
    X [list of MATRIX] - data
    """

    N = len(X)
    res = MATRIX(X[0]); res *= 0.0

    #===== Average ====
    for i in xrange(N):
        res = res + X[i]
    res = res / float(N)


    #===== Std ========
    res2 = MATRIX(X[0]); res2 *= 0.0

    for a in xrange(res2.num_of_rows):
        for b in xrange(res2.num_of_cols):
        
            tmp = 0.0
            for i in xrange(N):
                tmp = tmp + (X[i].get(a,b) - res.get(a,b))**2
            tmp = math.sqrt( tmp / float(N) )

            res2.set(a,b, tmp)

    # Find maximal and minimal values
    up_bound = MATRIX(X[0]); up_bound *= 0.0
    dw_bound = MATRIX(X[0]); dw_bound *= 0.0



    for a in xrange(res2.num_of_rows):
        for b in xrange(res2.num_of_cols):

            up_bound.set(a,b, X[0].get(a,b))
            dw_bound.set(a,b, X[0].get(a,b))

            for i in xrange(N):
                xab = X[i].get(a,b)
                if xab > up_bound.get(a,b):
                    up_bound.set(a,b, xab)
                if xab < dw_bound.get(a,b):
                    dw_bound.set(a,b,xab)


    return res, res2, dw_bound, up_bound


def cmat_stat(X):
    """
    Computes the average and std     
    X [list of CMATRIX] - data
    """

    N = len(X)
    res = CMATRIX(X[0]); res *= 0.0

    #===== Average ====
    for i in xrange(N):
        res = res + X[i]
    res = res / float(N)


    #===== Std ========
    res2 = CMATRIX(X[0]); res2 *= 0.0

    for a in xrange(res2.num_of_rows):
        for b in xrange(res2.num_of_cols):
        
            tmp = 0.0+0.0j
            for i in xrange(N):
                dx = X[i].get(a,b) - res.get(a,b)
                tmp = tmp + (dx.conjugate() * dx)
            tmp = math.sqrt( tmp / float(N) )

            res2.set(a,b, tmp)

    return res, res2





def printout(t, pops, Hvib, outfile):
    """
    t - time [a.u.] 
    pops - [MATRIX] - populations
    Hvib - [CMATRIX] - vibronic Hamiltonian
    outfile - filename where we'll print everything out
    """

    nstates = Hvib.num_of_cols


    line = "%8.5f " % (t)
    P, E = 0.0, 0.0
    for state in xrange(nstates):
        p, e = pops.get(state,0), Hvib.get(state, state).real
        P += p
        E += p*e
        line = line + " %8.5f  %8.5f " % (p, e)
    line = line + " %8.5f  %8.5f \n" % (P, E)

    f = open(outfile, "a") 
    f.write(line)
    f.close()



def show_matrix_splot(X, filename):
    ncol, nrow = X.num_of_cols, X.num_of_rows

    line = ""
    for i in xrange(nrow):
        for j in xrange(ncol):
            val = X.get(i,j)
            if i==j:
                val = 0.0
            line = line + "%4i %4i %8.5f \n" % (i, j, val)
        line = line + "\n"

    f = open(filename, "w")
    f.write(line)
    f.close()
 
    

def add_printout(i, pop, filename):
    # pop - CMATRIX(nstates, 1)

    f = open(filename,"a")
    line = "step= %4i " % i    

    tot_pop = 0.0
    for st in xrange(pop.num_of_cols):
        pop_o = pop.get(st,st).real
        tot_pop = tot_pop + pop_o
        line = line + " P(%4i)= %8.5f " % (st, pop_o)
    line = line + " Total= %8.5f \n" % (tot_pop)
    f.write(line)
    f.close()


