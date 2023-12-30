/**
 * Our raw data is in JSON format which is not convenient to sort.
 * We convert each line of JSON into an Item object and build a list of them
 * THis list can be sorted by a chosen field and then displayed
 */
class Item
{
public:
  Item(int at, const char *dest, const char *toward,int pn)
  {
    estArrivTime = at;
    platform = pn;
    destination = dest;
    towards = toward;
  };
  ~Item()
  {
    // Serial.println("Item destroyed");
  }
  int getArrivalTime()
  {
    return estArrivTime;
  }
  const char *getDestination()
  {
    return destination;
  }
  const char *getTowards()
  {
    return towards;
  }
  int getPlatform()
  {
    return platform;
  }

private:
  int estArrivTime; // seconds
  int platform;
  const char *destination;
  const char *towards;
};

