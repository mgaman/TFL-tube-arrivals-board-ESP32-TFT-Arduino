/**
 * Our raw data is in JSON format which is not convenient to sort.
 * We convert each line of JSON into an Item object and build a list of them
 * THis list can be sorted by a chosen field and then displayed
 */
class AP
{
public:
    AP(const char *ssid, int rssi)
    {
        strcpy(_ssid,ssid);
        _rssi = rssi;
    };
    ~AP()
    {
        // Serial.println("Item destroyed");
    }
    int getRSSI() {
        return _rssi;
    }
    const char *getSSID() {
        return _ssid;
    }
private:
    int _rssi;
    char _ssid[30];
};
