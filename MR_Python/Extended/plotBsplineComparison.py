from argparse import ArgumentParser
import os

import cPickle as pickle
import functions
import matplotlib.pyplot as plt
import numpy as np
import pysgpp

from functions import objFuncSGpp as objFuncSGpp

# Paper
# xticklabelsize = 16
# yticklabelsize = 16
# legendfontsize = 16
# titlefontsize = 18
# ylabelsize = 16
# xlabelsize = 18

# Presentation
xticklabelsize = 18
yticklabelsize = 18
legendfontsize = 24
titlefontsize = 22
ylabelsize = 20
xlabelsize = 22


def getColorAndMarker(gridType, refineType):
    if gridType == 'bspline':
        color = '#9467bd'; marker = '>';label = gridType
    elif gridType == 'bsplineBoundary':
        color = '#8c564b'; marker = '+';label = gridType
    elif gridType == 'modBspline':
        color = '#e377c2'; marker = 's';label = gridType
    elif gridType == 'bsplineClenshawCurtis':
        color = '#7f7f7f'; marker = 'h';label = gridType
    elif gridType == 'fundamentalSpline':
        color = '#bcbd22'; marker = '*';label = gridType
    elif gridType == 'modFundamentalSpline':
        color = '#17becf'; marker = 'd';label = gridType
    elif gridType == 'nakbspline':
        color = '#ff7f0e'; marker = 'h';label = '$b^{n,nak}_{l,i}$'
    elif gridType == 'nakbsplineboundary':
        color = '#1f77b4'; marker = 'x';label = '$b^{n,nak}_{l,i}$ boundary'     
    elif gridType == 'nakbsplinemodified':
        color = '#2ca02c'; marker = 'D';label = '$b^{n,mod}_{l,i}$'
    elif gridType == 'nakbsplineextended':
        color = '#d62728'; marker = 'o';label = '$b^{n,e}_{l,i}$'
        
    else:
        print("gridType {} not supported".format(gridType))
    
    if refineType in ['regular', 'regularByPoints']:
        label = label + ', regular'
    elif refineType == 'surplus':
        label = label + ', adaptive'
    
    return[color, marker, label]


def plotConvergenceOrder(order, start, length):
    X = np.linspace(start[0], start[0] * 10 ** length, 100)
    Y = [0] * len(X)
    for i in range(len(X)):
        Y[i] = X[i] ** (-order)
    linestyle = '-.'
    if order == 4:
        linestyle = '--'
    elif order == 6:
        linestyle = '-'
    name = '$h^{-' + '{}'.format(order) + '}$'
    label = name if name not in plt.gca().get_legend_handles_labels()[1] else ''
    plt.plot(X, Y, linestyle=linestyle, color='k' , label=label)


def plotter(qoi, data, refineTypes):
    gridSizes = data['gridSizes']
    refineType = data['refineType']
    if len(refineTypes) == 2 and 'regular' in refineType:
        linestyle = '--'
    else:
        linestyle = '-'
    if qoi == 'l2':
        interpolErrors = data['interpolErrors']
        
        # hack for SGA Paper: divide by max(MC Points)-min(MC Points) to get relative error.
        # This is necessary because the borehole function values are somewhere aroung 10^-6 
        # and thus the error looks smaller than it is
        # Update: Now nrmse does exactly this!
        # borehole:
        # interpolErrors /= (3.2478e-06 - 7.05594e-07)
        # exp:
        # interpolErrors /= (9.369084388257171 - 6.670584029966912)
        # Friedman:
        # interpolErrors /= (29.065382369280560 - 0.627873930461282)
        # monomials have max=1, min = 0.
        
        for i, gridType in enumerate(data['gridTypes']):
            [color, marker, label] = getColorAndMarker(gridType, refineType)
            plt.plot(gridSizes[i, :], interpolErrors[i, :], label=label, color=color, marker=marker, linestyle=linestyle)
            plt.gca().set_yscale('symlog', linthreshy=1e-16)  # in contrast to 'log', 'symlog' allows
            plt.gca().set_xscale('log')  # value 0 through small linearly scaled interval around 0
            # plt.ylabel('l2 error', fontsize=ylabelsize)
    
    elif qoi == 'nrmseWithOrder':
        interpolErrors = data['nrmsErrors']
        for i, gridType in enumerate(data['gridTypes']):
            [color, marker, label] = getColorAndMarker(gridType, refineType)
            plt.plot(gridSizes[i, :], interpolErrors[i, :], label=label, color=color, marker=marker, linestyle=linestyle)
            plt.gca().set_yscale('log') 
            plt.gca().set_xscale('log')
            # plt.ylabel('l2 error', fontsize=ylabelsize)
            if data['degree'] == 1:
                orders = [2]
            elif data['degree'] == 3:
                orders = [2, 4]
            elif data['degree'] == 5:
                orders = [2, 4, 6]
            for order in orders:
                plotConvergenceOrder(order, [1, 0], 2.5)
        
        if degree == 1:
            plt.ylabel(r'$\Vert u - \tilde{u} \Vert_2$', fontsize=ylabelsize)
                
    elif qoi == 'nrmse':
        nrmsErrors = data['nrmsErrors']
        for i, gridType in enumerate(data['gridTypes']):
            [color, marker, label] = getColorAndMarker(gridType, refineType)
            plt.plot(gridSizes[i, :], nrmsErrors[i, :], label=label, color=color, marker=marker, linestyle=linestyle)
            plt.gca().set_yscale('symlog', linthreshy=1e-16)  # in contrast to 'log', 'symlog' allows
            plt.gca().set_xscale('log')  # value 0 through small linearly scaled interval around 0
        if degree == 1:
            plt.ylabel(r'$\Vert u - \tilde{u} \Vert_2$', fontsize=ylabelsize)
            
    elif qoi == 'meanErr':
        meanErrors = data['meanErrors']
        for i, gridType in enumerate(data['gridTypes']):
            [color, marker, label] = getColorAndMarker(gridType, refineType)
            plt.loglog(gridSizes[i, :], meanErrors[i, :], label=label, color=color, marker=marker, linestyle=linestyle)
        if degree == 1:    
            plt.ylabel(r'$\vert E(u) - E(\tilde{u}) \vert$', fontsize=ylabelsize)
            
    elif qoi == 'varErr':
        varErrors = data['varErrors']
        for i, gridType in enumerate(data['gridTypes']):
            [color, marker, label] = getColorAndMarker(gridType, refineType)
            plt.loglog(gridSizes[i, :], varErrors[i, :], label=label, color=color, marker=marker, linestyle=linestyle)
        if degree == 1:
            plt.ylabel(r'$\vert V(u) - V(\tilde{u}) \vert$', fontsize=ylabelsize)
    else:
        print("qoi not supported")
    
    # plainE nrmseWithOrder
    # plt.ylim([1e-16, 3 * 1e-0])
    
    # boreholeUQ nrmse
    plt.ylim([1e-7, 1e-0])
    # boreholeUQ mean
    # plt.ylim([1e-10, 1e-2])
    # boreholeUQ var
    # plt.ylim([1e-12, 1e-4])
    
    plt.xlim([1e+0, 1e+4])
    
    plt.xlabel('number of grid points', fontsize=xlabelsize)
    # number of ticks
    plt.locator_params(axis='x', numticks=5)
    plt.locator_params(axis='y', numticks=5)
    # ticklabel size
    for tick in plt.gca().xaxis.get_major_ticks():
        tick.label.set_fontsize(xticklabelsize)
    for tick in plt.gca().yaxis.get_major_ticks():
        tick.label.set_fontsize(yticklabelsize) 


############################ Main ############################     
if __name__ == '__main__':
    # parse the input arguments
    parser = ArgumentParser(description='Get a program and run it with input', version='%(prog)s 1.0')
    parser.add_argument('--qoi', default='nrmse', type=str, help='what to plot')
    parser.add_argument('--model', default='boreholeUQ', type=str, help='define which test case should be executed')
    parser.add_argument('--dim', default=1, type=int, help='the problems dimensionality')
    parser.add_argument('--scalarModelParameter', default=0, type=int, help='purpose depends on actual model. For monomial its the degree')
    parser.add_argument('--degree', default=135, type=int, help='spline degree')
    parser.add_argument('--refineType', default='regularAndSurplus', type=str, help='surplus (adaptive) or regular')
    parser.add_argument('--maxLevel', default=8, type=int, help='maximum level for regualr refinement')
    parser.add_argument('--maxPoints', default=5000, type=int, help='maximum number of points used')
    parser.add_argument('--saveFig', default=1, type=int, help='save figure')
    
    # configure according to input
    args = parser.parse_args()
    
    if args.degree == 135:
        degrees = [1, 3, 5]
    elif args.degree == 35:
        degrees = [3, 5]
    else:
        degrees = [args.degree]
      
    if args.degree == 135:
        fig = plt.figure(figsize=(20, 4.5)) 
    else:
        fig = plt.figure()
    
    pyFunc = functions.getFunction(args.model, args.dim, args.scalarModelParameter)
    objFunc = objFuncSGpp(pyFunc)
    loadDirectory = os.path.join('/home/rehmemk/git/SGpp/MR_Python/Extended/data/', args.model, objFunc.getName())
    if args.refineType == 'regularAndSurplus':
        refineTypes = ['regularByPoints', 'surplus']
    elif args.refineType == 'regularLevelAndSurplus':
        refineTypes = ['regular', 'surplus']
    else:
        refineTypes = [args.refineType] 
    
    l = 1
    for degree in degrees:
        fig.add_subplot(1, len(degrees) , l)
        l += 1
        for refineType in refineTypes:
            if refineType == 'regular':
                saveName = objFunc.getName() + '_' + refineType + str(args.maxLevel)
            else:
                saveName = objFunc.getName() + refineType + str(args.maxPoints)
            plt.gca().set_title('n={}'.format(degree), fontsize=titlefontsize)
            datapath = os.path.join(loadDirectory, saveName + '_data{}.pkl'.format(degree))    
            with open(datapath, 'rb') as fp:
                data = pickle.load(fp)
                orders = [degree + 1]
                plotter(args.qoi, data, refineTypes)
    
    if args.saveFig == 1:
        # save in original folder
        saveDirectory = loadDirectory
        # save in paper figures folder 
        # saveDirectory = '/home/rehmemk/git/sga18proc/figures/'
        # plt.tight_layout() 
        if args.refineType == 'regular':
            saveName = objFunc.getName() + '_' + args.refineType + str(args.maxLevel) + '_' + args.qoi
        else:
            saveName = objFunc.getName() + args.refineType + str(args.maxPoints) + '_' + args.qoi
        figname = os.path.join(saveDirectory, saveName)
        plt.savefig(figname, dpi=300, bbox_inches='tight')
        print('saved fig to {}'.format(figname))
        # rrearrange legend order and save legends in individual files.
        legendstyle = 'external'
        if legendstyle == 'external':
            ax = plt.gca()
            handles, labels = ax.get_legend_handles_labels()
            ncol = 4
            
            originalHandles = handles[:]
            originalLabels = labels[:]
            boundaryInLegend = 1
            if boundaryInLegend == 1:
                handles[1] = originalHandles[4]; labels[1] = originalLabels[4];
                handles[2] = originalHandles[1]; labels[2] = originalLabels[1];
                handles[3] = originalHandles[5]; labels[3] = originalLabels[5];
                handles[4] = originalHandles[2]; labels[4] = originalLabels[2];
                handles[5] = originalHandles[6]; labels[5] = originalLabels[6];
                handles[6] = originalHandles[3]; labels[6] = originalLabels[3];
                ncol = 4
            else:
                handles[1] = originalHandles[3]; labels[1] = originalLabels[3];
                handles[2] = originalHandles[1]; labels[2] = originalLabels[1];
                handles[3] = originalHandles[4]; labels[3] = originalLabels[4];
                handles[4] = originalHandles[2]; labels[4] = originalLabels[2];
                ncol = 3

# for plainE
#             handles[2] = originalHandles[4]; labels[2] = originalLabels[4];
#             handles[3] = originalHandles[2]; labels[3] = originalLabels[2];
#             handles[4] = originalHandles[5]; labels[4] = originalLabels[5];
#             handles[5] = originalHandles[3]; labels[5] = originalLabels[3];
                 
            plt.figure()
            axe = plt.gca()
            axe.legend(handles, labels , loc='center', fontsize=legendfontsize, ncol=ncol)
            axe.xaxis.set_visible(False)
            axe.yaxis.set_visible(False)
            for v in axe.spines.values():
                v.set_visible(False)
            legendname = os.path.join(figname + '_legend')
            # cut off whitespace
            plt.subplots_adjust(left=0.0, right=1.0, top=0.6, bottom=0.4)
            plt.savefig(legendname, dpi=300, bbox_inches='tight', pad_inches=0.0)
        elif legendstyle == 'none':
            pass
        else:    
            plt.legend(ncol=4)
    else:
        plt.legend()
        plt.show()
