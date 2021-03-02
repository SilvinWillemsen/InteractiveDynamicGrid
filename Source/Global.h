/*
  ==============================================================================

    Global.h
    Created: 18 Feb 2021 2:42:57pm
    Author:  Silvin Willemsen

  ==============================================================================
*/

#pragma once
#include <fstream>

namespace Global
{
    static const int maxN = 200;
    static const int sliderHeight = 30;
    static const int margin = 5;
    
    static const bool useCVec = false;
    
    static std::vector<double> linspace (double start, double finish, int N)
    {
        std::vector<double> res (N, 0);
        for (int i = 0; i < N; ++i)
        {
            res[i] = start + i * (finish - start) / static_cast<double> (N - 1);
        }
        return res;
    }
    static double linspace (double start, double finish, int N, int idx)
    {
        if (idx >= N)
        {
            std::cout << "Idx is outside of range" << std::endl;
            return -1;
            
        }
        return start + idx * (finish - start) / static_cast<double> (N - 1);
    }
};
