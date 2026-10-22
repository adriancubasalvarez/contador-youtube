#pragma once
// Pantalla simulada para la vista previa en el PC: en lugar del ILI9341 por
// SPI, una imagen de 320x240 en la memoria. src/cyd/screen.cpp la usa en vez
// de la pantalla real cuando se compila con -DCYD_PREVIEW.

// La imagen de la pantalla, para que preview.cpp la guarde
inline LGFX_Sprite *&previewFrame()
{
    static LGFX_Sprite *frame = nullptr;
    return frame;
}

class CydDisplay : public LGFX_Sprite {
public:
    CydDisplay() { previewFrame() = this; }
    void init()
    {
        setColorDepth(16);
        createSprite(320, 240);
    }
    void setRotation(int) {}
    void setBrightness(int) {}
    template <typename T> int getTouch(T *, T *) { return 0; }
};
