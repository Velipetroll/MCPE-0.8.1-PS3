#pragma once
#include <AppPlatform.hpp>
#include <string>

struct ImageData;

class AppPlatform_ps3 : public AppPlatform {
public:
    AppPlatform_ps3();
    virtual ~AppPlatform_ps3();

    // Sobreescrituras virtuales puras y carga de imagenes
    virtual std::string getImagePath(const std::string& path, bool_t a4) override;
    virtual void loadPNG(ImageData& img, const std::string& path, bool_t a4) override;
    virtual void loadTGA(ImageData& img, const std::string& path, bool_t a4) override;

    // Sobreescritura para rastrear lectura de archivos
    virtual AssetFile readAssetFile(const std::string& path) override;
};
