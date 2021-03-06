/*
  ==============================================================================

    Dynamic1DWave.cpp
    Created: 18 Feb 2021 2:33:27pm
    Author:  Silvin Willemsen

  ==============================================================================
*/

#include <JuceHeader.h>
#include "Dynamic1DWave.h"

//==============================================================================
Dynamic1DWave::Dynamic1DWave (NamedValueSet& parameters, double k) : k (k),
c (*parameters.getVarPointer("c")),
L (*parameters.getVarPointer("L"))

{
    cToUse = c;
    
    h = c * k;
    
    N = L/h;
    if (N > Global::maxN)
    {
        std::cout << "Choose a higher c or lower L" << std::endl;
    }
    
    
    Nint = floor(N);
    
    alf = N - Nint;
    
    NintPrev = Nint;
    
    lambdaSq = c * c * k * k / (h * h);
    
    M = ceil (N * 0.5);
    Mw = floor (N * 0.5);

    uStates.reserve (3);
    wStates.reserve (3);
    
    for (int n = 0; n < 3; ++n) // include the boundaries
    {
        uStates.push_back (std::vector<double> (ceil (Global::maxN * 0.5) + 1, 0));
        wStates.push_back (std::vector<double> (floor (Global::maxN * 0.5) + 1, 0));
    }
    
    u.reserve (3);
    
    for (int n = 0; n < 3; ++n)
    {
        u.push_back (&uStates[n][0]);
        w.push_back (&wStates[n][0]);
    }
    
    quadIp.resize (3);
    customIp.resize (4);
    excite();
    
    uState.open ("uState.csv");
    wState.open ("wState.csv");
    alfSave.open ("alfSave.csv");
    MSave.open ("MSave.csv");
    MwSave.open ("MwSave.csv");

}

Dynamic1DWave::~Dynamic1DWave()
{
    closeFiles();
}

void Dynamic1DWave::paint (juce::Graphics& g)
{
    /* This demo code just fills the component's background and
       draws some placeholder text to get you started.

       You should replace everything in this method with your own
       drawing code..
    */

    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));   // clear the background

    g.setColour (Colours::cyan);
    double visualScaling = 500;
    Path stringPath = visualiseState (visualScaling, g);
    g.strokePath (stringPath, PathStrokeType(2.0f));
    
}

Path Dynamic1DWave::visualiseState (double visualScaling, Graphics& g)
{
    auto stringBounds = getHeight() / 2.0;
    Path stringPath;
    stringPath.startNewSubPath (0, stringBounds);
    int stateWidth = getWidth();
    auto spacing = stateWidth / static_cast<double>(N + 1);
    double x = 0;
    
    float newY;
    bool switchToW = false;
    for (int y = 0; y <= Nint+1; y++) // +1 for overlapping point
    {
        if (y <= M)
        {
            newY = -u[1][y] * visualScaling + stringBounds; // Needs to be -u, because a positive u would visually go down

        } else {
            if (!switchToW)
            {
                x -= spacing;
                x += alf * spacing;
                switchToW = true;
            }
            newY = -w[1][y - M - 1] * visualScaling + stringBounds; // Needs to be -u, because a positive u would visually go down
            
        }
        if (isnan(x) || isinf(abs(x) || isnan(newY) || isinf(abs(newY))))
        {
            std::cout << "Wait" << std::endl;
        };
        
        if (isnan(newY))
            newY = 0;
        stringPath.lineTo(x, newY);
        x += spacing;
        
    }
    stringPath.lineTo (stateWidth, stringBounds);
    return stringPath;
}

void Dynamic1DWave::resized()
{
    // This method is where you should set the bounds of any child
    // components that your component contains..

}

void Dynamic1DWave::calculate()
{
    recalculateCoeffs();
    calculateInterpolatedPoints();
//    lowPassConnection();
    calculateScheme();
    displacementCorrection();
}

void Dynamic1DWave::recalculateCoeffs()
{
    h = c * k;
    N = L / h;
    Nint = floor(N);
    lambdaSq = c * c * k * k / (h * h);

    alf = N - Nint;
    if (Nint != NintPrev)
    {
        if (abs(Nint - NintPrev) > 1)
            std::cout << "Too fast!" << std::endl;
        
        addRemovePoint();
    }
}

void Dynamic1DWave::calculateInterpolatedPoints()
{
    quadIp[0] = -(alf - 1) / (alf + 1);
    quadIp[1] = 1;
    quadIp[2] = (alf - 1) / (alf + 1);
    
    uMp1 = u[1][M] * quadIp[2]  + w[1][0] * quadIp[1] + w[1][1] * quadIp[0];
    wm1 = u[1][M-1] * quadIp[0] + u[1][M] * quadIp[1]  + w[1][0] * quadIp[2];

}

void Dynamic1DWave::lowPassConnection()
{
    double diffAtConn = w[1][0] - u[1][M];
    double lpCoeff = pow (1-alf, lpExponent);
    u[1][M] = u[1][M] + lpCoeff * diffAtConn * 0.5;
    w[1][0] = w[1][0] - lpCoeff * diffAtConn * 0.5;
}

void Dynamic1DWave::displacementCorrection()
{
    double etaDiv = 1.0;
    double epsilon = 0;
    double etaPrev = (w[2][0] - u[2][M]) * etaDiv;
    double sig0 = 1.0;
    
    double rForce = (1.0 - sig0 / k) / (1.0 + sig0 / k);
    double oOP = (h * (1.0 + sig0 / k) * (1.0 - alf)) / (2.0 * h * (alf + epsilon) + 2.0 * etaDiv * k * k * (1.0 + sig0 / k) * (1.0 - alf));
    
    double F = ((w[0][0] - u[0][M]) * etaDiv + rForce * etaPrev) * oOP;
    
    u[0][M] += k*k/h * F;
    w[0][0] -= k*k/h * F;
}

void Dynamic1DWave::calculateScheme()
{
    // calculate interpolated points
    
    // calculate u
    for (int l = 1; l < M; ++l)
        u[0][l] = 2 * u[1][l] - u[2][l] + lambdaSq * (u[1][l+1] - 2 * u[1][l] + u[1][l-1]);
    
    // calculate w
    for (int l = 1; l < Mw; ++l)
        w[0][l] = 2 * w[1][l] - w[2][l] + lambdaSq * (w[1][l+1] - 2 * w[1][l] + w[1][l-1]);
    
    // add interpolated points
    u[0][M] = 2 * u[1][M] - u[2][M] + lambdaSq * (uMp1 - 2 * u[1][M] + u[1][M-1]);
    
    w[0][0] = 2 * w[1][0] - w[2][0] + lambdaSq * (w[1][1] - 2 * w[1][0] + wm1);

}


void Dynamic1DWave::updateStates()
{
    double* uTmp = u[2];
    u[2] = u[1];
    u[1] = u[0];
    u[0] = uTmp;
    
    double* wTmp = w[2];
    w[2] = w[1];
    w[1] = w[0];
    w[0] = wTmp;
    
    NintPrev = Nint;
}

void Dynamic1DWave::addRemovePoint()
{
    if (Nint > NintPrev) // add point
    {
        alfTick = ((1.0-Mw * h) - ((M + 1) * h)) / h;
        std::cout << alf - alfTick << std::endl;
        
        customIp[0] = -alfTick * (alfTick + 1.0) / ((alfTick + 2.0) * (alfTick + 3.0));
        customIp[1] = 2.0 * alfTick / (alfTick + 2.0);
        customIp[2] = 2.0 / (alfTick + 2.0);
        customIp[3] = -2.0 * alfTick / ((alfTick + 3.0) * (alfTick + 2.0));
        
        if (Nint % 2 == 1)
        {   
            u[1][M+1] = customIp[0] * u[1][M-1]
                        + customIp[1] * u[1][M]
                        + customIp[2] * w[1][0]
                        + customIp[3] * w[1][1];
            
            u[2][M+1] = customIp[0] * u[2][M-1]
                        + customIp[1] * u[2][M]
                        + customIp[2] * w[2][0]
                        + customIp[3] * w[2][1];
            ++M;
            
        }
        else
        {
            // save w0 (and prev) beforehand, otherwise things will be overwritten
            double w0 = customIp[3] * u[1][M-1]
                        + customIp[2] * u[1][M]
                        + customIp[1] * w[1][0]
                        + customIp[0] * w[1][1];
            double w0Prev = customIp[3] * u[2][M-1]
                        + customIp[2] * u[2][M]
                        + customIp[1] * w[2][0]
                        + customIp[0] * w[2][1];
            
            // move w vector one up (can be optimised)
            for (int l = Mw; l >= 0; --l)
            {
                w[1][l+1] = w[1][l];
                w[2][l+1] = w[2][l];
                
            }
            w[1][0] = w0;
            w[2][0] = w0Prev;
            ++Mw;
        }
    } else {
        if (Nint % 2 == 0)
        {
            u[0][M] = 0;
            u[1][M] = 0;
            u[2][M] = 0;
            --M;
            
        }
        else
        {
            // move w vector one down (can be optimised)
            for (int l = 0; l <= Mw; ++l)
            {
                w[0][l] = w[0][l+1]; // apparently necessary
                w[1][l] = w[1][l+1];
                w[2][l] = w[2][l+1];
            }
            w[0][Mw] = 0;
            w[1][Mw] = 0;
            w[2][Mw] = 0;
            
            --Mw;
        }
    }
}

void Dynamic1DWave::excite()
{
    // Arbitrary excitation function. Just used this for testing purposes
    
    double width = floor(0.1 * M);
    double pos = 0.2;
    double loc = pos*N;
    int start = floor (loc-width*0.5);
    int end = std::min (M, static_cast<int>(start+width));
    
    // note the addition here
    
    for (int l = start; l < end; ++l)
    {
        u[1][l] += 0.5 * (1 - cos(2.0 * double_Pi * (l - start) / width));
        u[2][l] += 0.5 * (1 - cos(2.0 * double_Pi * (l - start) / width));
    }
}

void Dynamic1DWave::saveToFiles()
{
    alfSave << alf << ";\n";
    MSave << M << ";\n";
    MwSave << Mw << ";\n";
    
    for (int l = 0; l < uStates[0].size(); ++l)
    {
        if (l <= M)
            uState << u[1][l];
        else
            uState << 0;
        if (l != uStates[0].size()-1)
            uState << ", ";
    }
    uState << ";\n";

    for (int l = 0; l < wStates[0].size(); ++l)
    {
        if (l <= Mw)
            wState << w[1][l];
        else
            wState << 0;
        if (l != wStates[0].size()-1)
            wState << ", ";
    }
    wState << ";\n";
}

void Dynamic1DWave::closeFiles()
{
    uState.close();
    wState.close();
    alfSave.close();
    MSave.close();
    MwSave.close();

}
