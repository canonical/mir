// from android-input
#include <EventHub.h>

#include <list>
#include <map>

namespace mir
{

// An EventHub implementation that generates fake raw events.
class FakeEventHub : public android::EventHubInterface
{
public:
    FakeEventHub();
    virtual ~FakeEventHub();

    virtual uint32_t getDeviceClasses(int32_t deviceId) const;

    virtual android::InputDeviceIdentifier getDeviceIdentifier(int32_t deviceId) const;

    virtual void getConfiguration(int32_t deviceId,
            android::PropertyMap* outConfiguration) const;

    virtual android::status_t getAbsoluteAxisInfo(int32_t deviceId, int axis,
            android::RawAbsoluteAxisInfo* outAxisInfo) const;

    virtual bool hasRelativeAxis(int32_t deviceId, int axis) const;

    virtual bool hasInputProperty(int32_t deviceId, int property) const;

    virtual android::status_t mapKey(int32_t deviceId, int32_t scanCode, int32_t usageCode,
            int32_t* outKeycode, uint32_t* outFlags) const;

    virtual android::status_t mapAxis(int32_t deviceId, int32_t scanCode,
            android::AxisInfo* outAxisInfo) const;

    virtual void setExcludedDevices(const android::Vector<android::String8>& devices);

    virtual size_t getEvents(int timeoutMillis, android::RawEvent* buffer,
            size_t bufferSize);

    virtual int32_t getScanCodeState(int32_t deviceId, int32_t scanCode) const;
    virtual int32_t getKeyCodeState(int32_t deviceId, int32_t keyCode) const;
    virtual int32_t getSwitchState(int32_t deviceId, int32_t sw) const;
    virtual android::status_t getAbsoluteAxisValue(int32_t deviceId, int32_t axis,
                                          int32_t* outValue) const;

    virtual bool markSupportedKeyCodes(int32_t deviceId, size_t numCodes,
            const int32_t* keyCodes, uint8_t* outFlags) const;

    virtual bool hasScanCode(int32_t deviceId, int32_t scanCode) const;
    virtual bool hasLed(int32_t deviceId, int32_t led) const;
    virtual void setLedState(int32_t deviceId, int32_t led, bool on);

    virtual void getVirtualKeyDefinitions(int32_t deviceId,
            android::Vector<android::VirtualKeyDefinition>& outVirtualKeys) const;

    virtual android::sp<android::KeyCharacterMap> getKeyCharacterMap(int32_t deviceId) const;
    virtual bool setKeyboardLayoutOverlay(int32_t deviceId,
                                          const android::sp<android::KeyCharacterMap>& map);

    virtual void vibrate(int32_t deviceId, nsecs_t duration);
    virtual void cancelVibrate(int32_t deviceId);

    virtual void requestReopenDevices();

    virtual void wake();

    virtual void dump(android::String8& dump);

    virtual void monitor();

    // list of RawEvents available for consumption via getEvents
    std::list<android::RawEvent> events_available;

    typedef struct FakeDevice
    {
        uint32_t classes;
        android::InputDeviceIdentifier identifier;
        std::map<int, bool> input_properties;
    } FakeDevice;

    std::map<int32_t, FakeDevice> device_from_id;

};

} // namespace mir
