#include "objects/quad/quad.h"

using namespace love;

Quad::Quad(const Viewport& viewport, double sw, double sh) : common::Quad(sw, sh)
{
    this->Refresh(viewport, sw, sh);
}

void Quad::Refresh(const Viewport& viewport, double sw, double sh)
{
    this->RefreshViewport(viewport, sw, sh);
}

const Tex3DS_SubTexture& Quad::CalculateTex3DSViewport(const Viewport& viewport, C3D_Tex* texture)
{
    this->subTex.top  = 1.0f - (viewport.y) / texture->height;
    this->subTex.left = (viewport.x) / texture->width;

    this->subTex.right  = (viewport.x + viewport.w) / texture->width;
    this->subTex.bottom = 1.0f - ((viewport.y + viewport.h) / texture->height);

    this->subTex.width  = viewport.w;
    this->subTex.height = viewport.h;

    return this->subTex;
}
