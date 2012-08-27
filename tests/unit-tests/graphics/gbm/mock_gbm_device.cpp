#include <stdexcept>
#include <gbm.h>
#include "gbmint.h"

class MockGBMDevice;

static MockGBMDevice *thunk = NULL;

extern "C" {
static struct gbm_bo *mock_bo_create(struct gbm_device *gbm,
                            uint32_t width, uint32_t height,
                            uint32_t format,
                            uint32_t usage);

static void mock_bo_destroy(struct gbm_bo *bo);
}

class MockGBMDevice
{
public:
    MockGBMDevice()
    {
        using namespace testing;

        thunk = this;

        dev = std::shared_ptr<struct gbm_device>(new gbm_device());

        dev->bo_create = mock_bo_create;
        dev->bo_destroy = mock_bo_destroy;

        ON_CALL(*this, bo_create(_,_,_,_,_))
        .WillByDefault(Invoke(this, &MockGBMDevice::bo_create_impl));
    }

    MOCK_METHOD5(bo_create, struct gbm_bo *(struct gbm_device *, uint32_t, uint32_t, uint32_t, uint32_t));
    MOCK_METHOD1(bo_destroy, void(struct gbm_bo *));

    std::shared_ptr<struct gbm_device> get_device()
    {
        return dev;
    }
private:
    struct gbm_bo *bo_create_impl(struct gbm_device *dev, uint32_t w, uint32_t h, uint32_t format, uint32_t usage)
    {
        struct gbm_bo *bo = new gbm_bo();
        (void)format;
        (void)usage;
        bo->gbm = dev;
        bo->width = w;
        bo->height = h;
        switch (format) {
        case GBM_BO_FORMAT_ARGB8888:
            bo->stride = w * 4;
            break;
        default:
            throw std::runtime_error("Attempted to create buffer with unknown format");
        }
        return bo;
    }

    std::shared_ptr<struct gbm_device> dev;
};

extern "C" {
static struct gbm_bo *mock_bo_create(struct gbm_device *gbm,
                            uint32_t width, uint32_t height,
                            uint32_t format,
                            uint32_t usage)
{
    return thunk->bo_create(gbm, width, height, format, usage);
}

static void mock_bo_destroy(struct gbm_bo *bo)
{
    return thunk->bo_destroy(bo);
}
}
