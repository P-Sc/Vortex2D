//
//  Pressure.cpp
//  Vortex2D
//

#include "Pressure.h"

namespace Vortex2D { namespace Fluid {

namespace
{

const char* CommonFrag = GLSL(
    float fraction_inside(float a, float b)
    {
        if(a < 0.0 && b < 0.0)
            return 1.0;
        if(a < 0.0 && b >= 0.0)
            return a / (a - b);
        if(a >= 0.0 && b < 0.0)
            return b / (b - a);
        return 0.0;
    }

    vec2 get_weight(vec2 texCoord, ivec2 offset, sampler2D solid_phi)
    {
        vec2 weight;

        weight.x = 1.0 - fraction_inside(textureOffset(solid_phi, texCoord, offset + ivec2(0,2)).x,
                                         textureOffset(solid_phi, texCoord, offset).x);
        weight.y = 1.0 - fraction_inside(textureOffset(solid_phi, texCoord, offset + ivec2(2,0)).x,
                                         textureOffset(solid_phi, texCoord, offset).x);

        return clamp(weight, vec2(0.0), vec2(1.0));
    }
);

const char * DivFrag = GLSL(
    uniform sampler2D u_velocity;
    uniform sampler2D u_obstacles;
    uniform sampler2D u_fluid;
    uniform sampler2D u_obstacles_velocity;
    uniform float dx;

    in vec2 v_texCoord;
    out vec4 out_color;

    float fraction_inside(float a, float b);
    vec2 get_weight(vec2 texCoord, ivec2 offset, sampler2D solid_phi);

    void main()
    {
        float liquid_phi = texture(u_fluid, v_texCoord).x;
        if (liquid_phi < 0.0)
        {
            vec2  uv  = texture(u_velocity, v_texCoord).xy;
            float uxp = textureOffset(u_velocity, v_texCoord, ivec2(1,0)).x;
            float vyp = textureOffset(u_velocity, v_texCoord, ivec2(0,1)).y;

            vec2 wuv = get_weight(v_texCoord, ivec2(0,0), u_obstacles);
            float wxp = get_weight(v_texCoord, ivec2(2,0), u_obstacles).x;
            float wyp = get_weight(v_texCoord, ivec2(0,2), u_obstacles).y;

            float div = (wuv.x * uv.x - wxp * uxp + wuv.y * uv.y - wyp * vyp) / dx;

            // FIXME what should the value of c be?

            /*
            vec2 solid_uv = texture(u_obstacles_velocity, v_texCoord).xy;
            float solid_uxp = textureOffset(u_obstacles_velocity, v_texCoord, ivec2(1,0)).x;
            float solid_vyp = textureOffset(u_obstacles_velocity, v_texCoord, ivec2(0,1)).y;

            div += ((h_cxp+c-1.0)*solid_uxp - (h_cxn+c-1.0)*solid_uv.x + (h_cyp+c-1.0)*solid_vyp - (h_cyn+c-1.0)*solid_uv.y) / dx;
            */

            //pressure, div, 0, 0
            out_color = vec4(0.0, div, 0.0, 0.0);
        }
        else
        {
            out_color = vec4(0.0);
        }
    }
);

const char * ProjectFrag = GLSL(
    in vec2 v_texCoord;
    out vec4 out_color;

    uniform sampler2D u_velocity;
    uniform sampler2D u_pressure;
    uniform sampler2D u_fluid;
    uniform sampler2D u_obstacles;
    uniform sampler2D u_obstacles_velocity;
    uniform float delta;

    float fraction_inside(float a, float b);

    void main()
    {
        vec2 cell = texture(u_velocity, v_texCoord).xy;

        float p = texture(u_pressure, v_texCoord).x;
        float pxn = textureOffset(u_pressure, v_texCoord, ivec2(-1,0)).x;
        float pyn = textureOffset(u_pressure, v_texCoord, ivec2(0,-1)).x;

        vec2 pGrad = vec2(p-pxn, p-pyn);

        float phi = texture(u_fluid, v_texCoord).x;
        float phixn = textureOffset(u_fluid, v_texCoord, ivec2(-1,0)).x;
        float phiyn = textureOffset(u_fluid, v_texCoord, ivec2(0,-1)).x;

        vec2 theta = vec2(fraction_inside(phi, phixn), fraction_inside(phi, phiyn));
        pGrad /= max(theta, 0.01);

        vec2 mask = vec2(1.0);
        vec2 obsV = vec2(0.0);

        if (textureOffset(u_obstacles, v_texCoord, ivec2(2,0)).x > 0.0)
        {
            mask.x = 0.0;
            obsV.x = textureOffset(u_obstacles_velocity, v_texCoord, ivec2(1,0)).x;
        }
        if (textureOffset(u_obstacles, v_texCoord, ivec2(-2,0)).x > 0.0)
        {
            mask.x = 0.0;
            obsV.x = textureOffset(u_obstacles_velocity, v_texCoord, ivec2(-1,0)).x;
        }
        if (textureOffset(u_obstacles, v_texCoord, ivec2(0,2)).x > 0.0)
        {
            mask.y = 0.0;
            obsV.y = textureOffset(u_obstacles_velocity, v_texCoord, ivec2(0,1)).y;
        }
        if (textureOffset(u_obstacles, v_texCoord, ivec2(0,-2)).x > 0.0)
        {
            mask.y = 0.0;
            obsV.y = textureOffset(u_obstacles_velocity, v_texCoord, ivec2(0,-1)).y;
        }

        float dx = 1.0;
        vec2 new_cell = cell - delta * pGrad / dx;
        out_color = vec4(mask * new_cell + obsV, 0.0, 0.0);
    }
);

const char * WeightsFrag = GLSL(
    in vec2 v_texCoord;
    out vec4 out_color;

    uniform sampler2D u_obstacles;
    uniform sampler2D u_fluid;
    uniform float delta;
    uniform float dx;

    float fraction_inside(float a, float b);
    vec2 get_weight(vec2 texCoord, ivec2 offset, sampler2D solid_phi);

    void main()
    {
        float liquid_phi = texture(u_fluid, v_texCoord).x;
        if (liquid_phi < 0.0)
        {
            float pxp = textureOffset(u_fluid, v_texCoord, ivec2(1,0)).x;
            float pxn = textureOffset(u_fluid, v_texCoord, ivec2(-1,0)).x;
            float pyp = textureOffset(u_fluid, v_texCoord, ivec2(0,1)).x;
            float pyn = textureOffset(u_fluid, v_texCoord, ivec2(0,-1)).x;

            vec4 weights;
            vec2 uv = get_weight(v_texCoord, ivec2(0,0), u_obstacles);
            weights.x = pxp >= 0.0 ? 0.0 : -get_weight(v_texCoord, ivec2(2,0), u_obstacles).x;
            weights.y = pxn >= 0.0 ? 0.0 : -uv.x;
            weights.z = pyp >= 0.0 ? 0.0 : -get_weight(v_texCoord, ivec2(0,2), u_obstacles).y;
            weights.w = pyn >= 0.0 ? 0.0 : -uv.y;

            out_color = delta * weights / (dx*dx);
        }
        else
        {
            out_color = vec4(0.0);
        }
    }
);

const char * DiagonalsFrag = GLSL(
    in vec2 v_texCoord;
    out vec4 out_color;

    uniform sampler2D u_obstacles;
    uniform sampler2D u_fluid;
    uniform float delta;
    uniform float dx;

    float fraction_inside(float a, float b);
    vec2 get_weight(vec2 texCoord, ivec2 offset, sampler2D solid_phi);

    void main()
    {
        float liquid_phi = texture(u_fluid, v_texCoord).x;
        if (liquid_phi < 0.0)
        {
            float pxp = textureOffset(u_fluid, v_texCoord, ivec2(1,0)).x;
            float pxn = textureOffset(u_fluid, v_texCoord, ivec2(-1,0)).x;
            float pyp = textureOffset(u_fluid, v_texCoord, ivec2(0,1)).x;
            float pyn = textureOffset(u_fluid, v_texCoord, ivec2(0,-1)).x;

            vec4 weights;
            vec2 uv = get_weight(v_texCoord, ivec2(0,0), u_obstacles);
            weights.x = get_weight(v_texCoord, ivec2(2,0), u_obstacles).x;
            weights.y = uv.x;
            weights.z = get_weight(v_texCoord, ivec2(0,2), u_obstacles).y;
            weights.w = uv.y;

            vec4 theta;
            theta.x = pxp < 0.0 ? 1.0 : fraction_inside(liquid_phi, pxp);
            theta.y = pxn < 0.0 ? 1.0 : fraction_inside(liquid_phi, pxn);
            theta.z = pyp < 0.0 ? 1.0 : fraction_inside(liquid_phi, pyp);
            theta.w = pyn < 0.0 ? 1.0 : fraction_inside(liquid_phi, pyn);

            weights /= max(theta, 0.01);

            out_color = vec4(delta * dot(weights, vec4(1.0)) / (dx*dx), 0.0, 0.0, 0.0);
        }
        else
        {
            out_color = vec4(0.0);
        }
    }
);

}

using Renderer::Back;

Pressure::Pressure(float dt,
                   const glm::vec2& size,
                   LinearSolver& solver,
                   LinearSolver::Data& data,
                   Renderer::Buffer& velocity,
                   Renderer::Buffer& solidPhi,
                   Renderer::Buffer& liquidPhi,
                   Renderer::Buffer& solidVelocity)
    : mSolver(solver)
    , mData(data)
    , mVelocity(velocity)
    , mSolidPhi(solidPhi)
    , mLiquidPhi(liquidPhi)
    , mSolidVelocity(solidVelocity)
    , mDiv(Renderer::Shader::TexturePositionVert, DivFrag, CommonFrag)
    , mProject(Renderer::Shader::TexturePositionVert, ProjectFrag, CommonFrag)
    , mWeights(Renderer::Shader::TexturePositionVert, WeightsFrag, CommonFrag)
    , mDiagonals(Renderer::Shader::TexturePositionVert, DiagonalsFrag, CommonFrag)
{
    mDiv.Use().Set("u_velocity", 0).Set("u_obstacles", 1).Set("u_fluid", 2).Set("u_obstacles_velocity", 3).Set("dx", 1.0f / size.x).Unuse();
    mProject.Use().Set("u_velocity", 0).Set("u_pressure", 1).Set("u_fluid", 2).Set("u_obstacles", 3).Set("u_obstacles_velocity", 4).Set("delta", dt).Unuse();
    mWeights.Use().Set("u_obstacles", 0).Set("u_fluid", 1).Set("delta", dt).Set("dx", 1.0f / size.x).Unuse();
    mDiagonals.Use().Set("u_obstacles", 0).Set("u_fluid", 1).Set("delta", dt).Set("dx", 1.0f /size.x).Unuse();
}

void Pressure::Solve(LinearSolver::Parameters& params)
{

    mData.Pressure = mDiv(mVelocity, mSolidPhi, mLiquidPhi, mSolidVelocity);
    mData.Weights = mWeights(mSolidPhi, mLiquidPhi);
    mData.Diagonal = mDiagonals(mSolidPhi, mLiquidPhi);

    mSolver.Init(mData);
    mSolver.Solve(mData, params);

    mVelocity.Swap();
    mVelocity = mProject(Back(mVelocity),
                         mData.Pressure,
                         mLiquidPhi,
                         mSolidPhi,
                         mSolidVelocity);
}

}}