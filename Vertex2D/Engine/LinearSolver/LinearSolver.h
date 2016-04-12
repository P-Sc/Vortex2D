//
//  LinearSolver.h
//  Vertex2D
//
//  Created by Maximilian Maldacker on 19/08/2015.
//  Copyright (c) 2015 Maximilian Maldacker. All rights reserved.
//

#ifndef Vertex2D_LinearSolver_h
#define Vertex2D_LinearSolver_h

#include "Operator.h"

namespace Fluid
{

struct LinearSolver
{
    struct Data
    {
        Data(const glm::vec2 & size)
        : Weights(size, 4)
        , Diagonal(size, 1)
        , Pressure(size, 2, true, true)
        {}

        Buffer Weights;
        Buffer Diagonal;
        Buffer Pressure;
    };

    virtual void Init(Data & data) = 0;
    virtual void Solve(Data & data) = 0;
};

}

#endif
