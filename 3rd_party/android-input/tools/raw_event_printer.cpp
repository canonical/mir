#include <EventHub.h>
#include <iostream>

static void printRawEvent(const android::RawEvent &raw_event)
{
  std::cout << "==== RawEvent ====" << std::endl;
  std::cout << "when    : " << raw_event.when << std::endl;
  std::cout << "deviceId: " << raw_event.deviceId << std::endl;
  std::cout << "type    : " << raw_event.type << std::endl;
  std::cout << "code    : " << raw_event.code << std::endl;
  std::cout << "value   : " << raw_event.value << std::endl;
  std::cout << "==================" << std::endl;
  std::cout << std::endl;
}

int main()
{
  android::EventHub* event_hub = new android::EventHub;
  static const size_t buffer_size = 100;
  android::RawEvent raw_events[buffer_size];

  size_t events_read;
  do
  {
    events_read = event_hub->getEvents(10000, raw_events, buffer_size);
    for (size_t i = 0; i < events_read; ++i)
    {
      printRawEvent(raw_events[i]);
    }
  }
  while(events_read > 0);
  
  return 0;
}
