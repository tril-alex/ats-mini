#include "Common.h"
#include "EIBI.h"

#include <HTTPClient.h>
#include <WiFi.h>
#include <LittleFS.h>
#include <FS.h>

#include <ctype.h>
#include <string.h>

#define EIBI_PATH "/schedules.bin"
#define TEMP_PATH "/schedules.tmp"
#define EIBI_URL  "http://eibispace.de/dx/eibi.txt"

const BandLabel bandLabels[] =
{
  {  472,   479,  "630m (CW)"     },
  {  500,   518,  "NAVTEX"        },
  {  525,  1705,  "MW Broadcast"  },
  { 1600,  1800,  "Top Band"      },
  { 2300,  2495,  "120m BC"       },
  { 3200,  3400,  "90m BC"        },
  { 3500,  3800,  "80m (Amateur)" },
  { 3700,  3800,  "80m (SSB)"     },
  { 3900,  4000,  "75m BC"        },
  { 4720,  4750,  "60m (Amateur)" },
  { 4750,  4995,  "60m BC"        },
  { 5900,  6200,  "49m BC"        },
  { 7000,  7040,  "40m (CW)"      },
  { 7040,  7100,  "40m (DIGI)"    },
  { 7100,  7200,  "40m (SSB)"     },
  { 7200,  7600,  "41m BC"        },
  { 9400,  9900,  "31m BC"        },
  {10100, 10140,  "30m (CW)"      },
  {10136, 10140,  "30m (FT8)"     },
  {10140, 10150,  "30m (DIGI)"    },
  {11600, 12100,  "25m BC"        },
  {14000, 14070,  "20m (CW)"      },
  {14070, 14100,  "20m (DIGI/FT8)"},
  {14100, 14350,  "20m (SSB)"     },
  {15100, 15800,  "19m BC"        },
  {18068, 18110,  "17m (CW)"      },
  {18100, 18110,  "17m (FT8)"     },
  {18110, 18168,  "17m (SSB)"     },
  {17480, 17900,  "16m BC"        },
  {21000, 21070,  "15m (CW)"      },
  {21070, 21100,  "15m (DIGI/FT8)"},
  {21100, 21450,  "15m (SSB)"     },
  {21450, 21850,  "13m BC"        },
  {24890, 24915,  "12m (CW)"      },
  {24915, 24925,  "12m (FT8)"     },
  {24925, 24990,  "12m (SSB)"     },
  {26960, 27410,  "11m (CB)"      },
  {25670, 26100,  "11m BC"        },
  {28000, 28120,  "10m (CW)"      },
  {28120, 28190,  "10m (DIGI/FT8)"},
  {28200, 29700,  "10m (SSB/FM)"  },
  {26100, 26500,  "11m BC"        },
  {29600, 30000,  "9m BC"         }
};

const StationSchedule *eibiLookup(uint16_t freq, uint8_t hour, uint8_t minute)
{
  // Will return this static entry
  static StationSchedule entry;

  // Open file with EIBI data
  fs::File file = LittleFS.open(EIBI_PATH, "rb");
  if(!file) return(NULL);

  // Set up binary search
  ssize_t left  = 0;
  ssize_t right = file.size() / sizeof(entry);
  ssize_t mid;

  // Search for the frequency
  while(left <= right)
  {
    // Go to the middle entry
    mid = (left + right) / 2;
    file.seek(mid * sizeof(entry), fs::SeekSet);

    // Read middle entry
    if(file.read((uint8_t*)&entry, sizeof(entry)) != sizeof(entry))
    {
      file.close();
      return(NULL);
    }

    // Compare frequency
    if(entry.freq == freq)
      break;
    if(entry.freq < freq)
      left = mid + 1;
    else if(entry.freq > freq)
      right = mid - 1;
  }

  // Drop out if not found
  if(left > right)
  {
    file.close();
    return(NULL);
  }

  // This is our current time in minutes
  int now = hour * 60 + minute;

  // Keep reading entries from file
  do
  {
    // Match frequency
    if(entry.freq != freq) break;

    // If entry applies to all hours, return it
    if(entry.start_h < 0 || entry.end_h < 0)
    {
      file.close();
      return(&entry);
    }

    // These are starting/ending times in minutes
    int start = entry.start_h * 60 + entry.start_m;
    int end   = entry.end_h * 60 + entry.end_m;

    // Check for inclusive schedule
    if(start <= end && now >= start && now <= end)
    {
      file.close();
      return(&entry);
    }

    // Check for exclusive schedule
    if(start > end && (now >= start || now <= end))
    {
      file.close();
      return(&entry);
    }
  }
  while(file.read((uint8_t*)&entry, sizeof(entry)) == sizeof(entry));

  // Not found
  file.close();
  return(NULL);
}

static bool eibiParseLine(const char *line, StationSchedule &entry)
{
  char nameStr[sizeof(entry.name) + 1];
  char freqStr[12] = {0};
  char timeStr[12] = {0};
  char tmpCol[8]   = {0};
  char *p, *t;

  // Scan line for data
  if(sscanf(line, "%11c%11c%7c%24c", freqStr, timeStr, tmpCol, nameStr)<3)
    return(false);

  // Terminate found data
  freqStr[11] = '\0';
  timeStr[11] = '\0';
  nameStr[24] = '\0';

  // Parse frequency
  entry.freq = (uint16_t)atof(freqStr);
  if(!entry.freq) return(false);

  // Parse time
  int sh, sm, eh, em;
  if(sscanf(timeStr, "%2d%2d-%2d%2d", &sh, &sm, &eh, &em) != 4) return(false);
  entry.start_h = sh;
  entry.start_m = sm;
  entry.end_h   = eh;
  entry.end_m   = em;

  // Remove leading and trailing white space from name
  for(p = nameStr ; *p==' ' || *p=='\t' ; ++p);
  for(t = p + strlen(p) - 1 ; t>=p && (*t==' ' || *t=='\t') ; *t--='\0');

  // Copy name
  strncpy(entry.name, p, sizeof(entry.name) - 1);
  entry.name[sizeof(entry.name)-1] = '\0';

  // Done
  return(true);
}

bool eibiLoadSchedule()
{
  static const char *eibiMessage = "Loading EIBI Schedule";
  HTTPClient http;

  drawWiFiStatus(eibiMessage, "Connecting...");

  // Open HTTP connection to EIBI site
  http.begin(EIBI_URL);
  if(http.GET() != HTTP_CODE_OK)
  {
    drawWiFiStatus(eibiMessage, "Failed connecting to EIBI!");
    http.end();
    return(false);
  }

  // Open file in the local flash file system
  fs::File file = LittleFS.open(TEMP_PATH, "wb");
  if(!file)
  {
    drawWiFiStatus(eibiMessage, "Failed opening local storage!");
    http.end();
    return(false);
  }

  // Start loading data
  WiFiClient *stream = http.getStreamPtr();
  int totalLen = http.getSize();
  int byteCnt, lineCnt, charCnt;
  char charBuf[200];

  for(byteCnt = charCnt = lineCnt = 0 ; http.connected() && (totalLen<0 || byteCnt<totalLen) ; )
  {
    if(!stream->available()) delay(1);
    else
    {
      char c = stream->read();
      byteCnt++;

      if(c!='\n' && charCnt<sizeof(charBuf)-1) charBuf[charCnt++] = c;
      else
      {
        char *p, *t;

        // Remove whitespace
        charBuf[charCnt] = '\0';
        for(p = charBuf ; *p && *p<=' ' ; ++p);
        for(t = charBuf + charCnt - 1 ; t>=p && *t<=' ' ; *t--='\0');

        // If valid non-empty schedule line...
        if(t-p+1>0 && isdigit(*p))
        {
          // Remove LFs
          for(t = p ; *t ; ++t)
            if(*t=='\r') *t = ' ';

          // If parsed a new entry...
          StationSchedule entry;
          if(eibiParseLine(p, entry))
          {
            // Write it to the output file
            file.write((uint8_t*)&entry, sizeof(entry));
            lineCnt++;

            if(!(lineCnt % 100))
            {
              char statusMessage[64];
              sprintf(statusMessage, "... %d bytes, %d entries ...", byteCnt, lineCnt);
              drawWiFiStatus(eibiMessage, statusMessage);
            }
          }
        }

        // Done with the current buffer, start a new one
        charCnt = 0;
        if(c!='\n') charBuf[charCnt++] = c;
      }
    }
  }

  drawWiFiStatus(eibiMessage, "DONE!");

  // Done with file and HTTP connection
  file.close();
  http.end();

  // Move new schedule to its permanent place
  LittleFS.remove(EIBI_PATH);
  LittleFS.rename(TEMP_PATH, EIBI_PATH);

  // Success
  return(true);
}
