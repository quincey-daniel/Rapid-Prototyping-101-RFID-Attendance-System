/*
 * ESP32 RFID Attendance Tracking System
 * Tracks student attendance using RFID cards and Google Sheets
 * Hardware: ESP32, MFRC522 RFID Reader
 */

#include <Arduino.h>
#include <SPI.h>
#include <MFRC522v2.h>
#include <MFRC522DriverSPI.h>
#include <MFRC522DriverPinSimple.h>
#include <MFRC522Constants.h>
#include <WiFi.h>
#include <ESP_Google_Sheet_Client.h>

// System Configuration
struct Config {
    static const int MAX_STUDENTS = 5;
    
    static const uint8_t RFID_RST_PIN = 0;
    static const uint8_t RFID_SS_PIN = 5;
    
    static const int SCAN_DELAY_MS = 1000;
    static const int WIFI_RETRY_DELAY_MS = 1000;
    static const int TOKEN_REFRESH_MINUTES = 10;
};

// Network Credentials
struct Credentials {
    static constexpr char WIFI_SSID[] = "Q8668";
    static constexpr char WIFI_PASSWORD[] = "fuckofff";

    static constexpr char GOOGLE_PROJECT_ID[] = "rollcall-441720";
    static constexpr char GOOGLE_CLIENT_EMAIL[] = "attendanceofficer@rollcall-441720.iam.gserviceaccount.com";
    static constexpr char GOOGLE_PRIVATE_KEY[] = "-----BEGIN PRIVATE KEY-----\nMIIEvQIBADANBgkqhkiG9w0BAQEFAASCBKcwggSjAgEAAoIBAQDE036ODNjxouyp\nOxTi969fHHp5MvV02Dxk06TyA0H2cSL9Kzhrm/eQFNL3ElwyJC8G3+2J4lA+IOkv\nkj6yvsZxgAFBl7cjr+gmGaWc/tVAXCl1DsOrjZEkR25MIHGclYsEVo+1ZtLju0Ox\nzrU5TXcqNCVl1Qn4f2dS27QWhX6mhHUg5uS3r1uiD9pdBpvdXjmGkRKVPLVui4MD\nhshDW6q612o5xRG9Lk5kTMnhM9wNG42FwwzK6p1JtppmjQiOO6iEQFSDBEN0YWFC\nM8Ov9j8lY0/SaN3E+DEH9V5slIrjQ1IuSgWC1oRfZn/VfGU2mVrib3oX8syD85jt\npdZCJm//AgMBAAECggEAFOp2AA8PcsYh024ALzUA7DYw3EufrTYEB6GSCAHJSFtc\ny33Z9ggnS6awMH9BUDbAz0CL6TjP1s+qgq/40l0sO114mgl5oGTjYkEcH03ZSDND\nAznOw0k89kJST0FXXBDBgB7ZZf6Hme8JfONUJhhm+b4m3JEXXn0zc/5S6d3FHVlg\nVsMGl0faFVrCtIDja6jNinfkmFQACTOdLwbH4wNFwUsc2eZrrBGvMucgzL0BB8od\nZRNYoCsDbL26FMkV5c6nL7SxLSeo4+qHDrLCAgPdrprINvkwxhQ5cBx5EpIu/JdJ\n0qQ0WJ3yjKCmJI16JurIpC/2j2EQ5gfRV7lTdPTgmQKBgQDpRewpe6y6M1uHVjlI\nWMlsKI85lxH51Hx854O3Coz72Mb+CA6LroXZnwQ8oFl51W25mCMjQnncLkD2l/11\nHD1ezwl80X+5ql0suEBJ4Jw7/XI9biyQUSF1/vlCzrwx5JFVKcHYAmea/jfdF0pS\nBL4g2IqC8zajAG/2HGw4bbKoowKBgQDYAIux38p2To75qy1w2xD6Yv2OiTpRQFsT\nZH0+hCgpkzval6jQ7dTmW6a9Ea/DqQNYq1JJ7qSZXivJLxVHccbq0gZ472Bdsbph\n5zCzwAP+7Hk4dUOzSevyJgoq3q2WyEhBSyHXh5QB4GWWEOeKBW3++w70AaTofwpB\n2jTUzF2E9QKBgDM0qByLC2VsNKFGqhhZdQ2K8bWglc+Tdygr4dviMwRtl3DX7+l1\ni+gzYci1Ii7+TLG61au9weY1OQloX7i/VDFlnR2LF1B4Ny/D2kjdRy5b+iHF935O\ncNvn9mtV2jXRiJ17JxP1oyyAtV6Q6D2M0PeA0RbhsJKjW/BYWOEiI1+lAoGAH5Wc\n16qgoUIFeA6Gu3zc86/r4Z7BT0Y4yxIjmjOa59FmrCUHA13zhqeaLwVaDSM2oQ2U\nH8lsehyiDG39D5BchNOnLKHcFnc5iL1UisQmEW2koBPNjXbesG7Nh8091tF8d0e0\nMB7cDpYwZIwePzliBynQ2u8unOyOpGhy1zb26c0CgYEAyhtl7MxiBxVYijHP/zZW\nhaYyobt/lmrJnm90TT7OaRAz9l11EMI7p0U06zUdd07XpUFsxkrR3Sq+/F5SZPi3\nM+1hzCokZFg8cH48KncrDU37rSd2PigeIe4035aLu6xgOHzbKBCUFGccMBVzANZq\n+X0/K4/8defS1NAgY4ow5GA=\n-----END PRIVATE KEY-----\n";
    
    static constexpr char SPREADSHEET_ID[] = "1zwNPWRQ1-pmZrw9XcCbqFkKCSCy_TyJliiqtH1_RV2E";
};

// Student Database
class StudentDatabase {
public:
    static const String firstNames[100];
    static const String lastNames[100];
    
    static String getFullName(int index) {
        if (index >= 0 && index < 100) {
            return firstNames[index] + " " + lastNames[index];
        }
        return "Invalid Student";
    }
};

// RFID Reader Setup
MFRC522DriverPinSimple ss_pin(Config::RFID_SS_PIN);
MFRC522DriverSPI driver{ss_pin};
MFRC522 rfidReader{driver};

// System State
bool isSystemInitialized = false;

// Function Declarations
bool initializeWiFi();
bool initializeGoogleSheets();
bool initializeRFID();
void handleNewCard();
void updateAttendance(int studentIndex);
void initializeSpreadsheet();
void clearSpreadsheetData();

// WiFi Connection Handler
bool initializeWiFi() {
    Serial.print("Connecting to WiFi");
    WiFi.setAutoReconnect(true);
    WiFi.begin(Credentials::WIFI_SSID, Credentials::WIFI_PASSWORD);

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(Config::WIFI_RETRY_DELAY_MS);
        Serial.print(".");
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("\nConnected to WiFi. IP: %s\n", WiFi.localIP().toString().c_str());
        return true;
    }
    
    Serial.println("\nWiFi connection failed!");
    return false;
}

// Google Sheets Authentication Handler
void tokenStatusCallback(TokenInfo info) {
    if (info.status == token_status_error) {
        Serial.printf("Token Error: %s\n", GSheet.getTokenError(info).c_str());
    }
}

bool initializeGoogleSheets() {
    GSheet.printf("ESP Google Sheet Client v%s\n", ESP_GOOGLE_SHEET_CLIENT_VERSION);
    
    // Configure Google Sheets client
    GSheet.setTokenCallback(tokenStatusCallback);
    GSheet.setPrerefreshSeconds(Config::TOKEN_REFRESH_MINUTES * 60);
    
    // Initialize authentication
    GSheet.begin(Credentials::GOOGLE_CLIENT_EMAIL, 
                 Credentials::GOOGLE_PROJECT_ID, 
                 Credentials::GOOGLE_PRIVATE_KEY);
    
    return GSheet.ready();
}

// Spreadsheet Operations
void clearSpreadsheetData() {
    FirebaseJson response;
    bool success = GSheet.values.clear(&response, 
                                     Credentials::SPREADSHEET_ID, 
                                     "Sheet1!A:Z");
    
    Serial.println(success ? "Spreadsheet cleared successfully" 
                         : "Failed to clear spreadsheet");
}

void addHeaders() {
   FirebaseJson response, data;
    
    data.add("majorDimension", "ROWS");
    data.set("values/[0]/[0]", "Last Name");
    data.set("values/[0]/[1]", "First Name");
    data.set("values/[0]/[2]", "Attendance");

    bool success = GSheet.values.append(&response, 
                                      Credentials::SPREADSHEET_ID, 
                                      "Sheet1!A:C", 
                                      &data);

    if (!success) {
        Serial.printf("Failed to add headers!");
    }
}

void initializeSpreadsheet() {
    clearSpreadsheetData();
    Serial.println("Initializing attendance sheet...");

    addHeaders();
    for (int i = 0; i < Config::MAX_STUDENTS; i++) {
        FirebaseJson response, data;
        
        data.add("majorDimension", "ROWS");
        data.set("values/[0]/[0]", StudentDatabase::lastNames[i]);
        data.set("values/[0]/[1]", StudentDatabase::firstNames[i]);
        data.set("values/[0]/[2]", 0);

        bool success = GSheet.values.append(&response, 
                                          Credentials::SPREADSHEET_ID, 
                                          "Sheet1!A:C", 
                                          &data);

        if (!success) {
            Serial.printf("Failed to add student %d: %s\n", 
                         i, GSheet.errorReason().c_str());
        }
    }
}

// Attendance Management
void updateAttendance(int studentIndex) {
    if (studentIndex < 0 || studentIndex >= Config::MAX_STUDENTS) {
        Serial.println("Invalid student index");
        return;
    }

    FirebaseJson response, readData, updateData;
    String cellRange = "Sheet1!C" + String(studentIndex + 2);
    
    // Read current attendance count
    bool readSuccess = GSheet.values.get(&readData, 
                                       Credentials::SPREADSHEET_ID, 
                                       cellRange);
    
    int currentCount = 0;
    if (readSuccess) {
        FirebaseJsonData result;
        if (readData.get(result, "values/[0]/[0]")) {
            currentCount = result.stringValue.toInt();
        }
    }

    // Update attendance count
    updateData.add("majorDimension", "ROWS");
    updateData.set("values/[0]/[0]", String(currentCount + 1));

    bool updateSuccess = GSheet.values.update(&response, 
                                            Credentials::SPREADSHEET_ID, 
                                            cellRange, 
                                            &updateData);

    if (updateSuccess) {
        Serial.printf("Updated attendance for %s: %d\n", 
                     StudentDatabase::getFullName(studentIndex).c_str(), 
                     currentCount + 1);
    } else {
        Serial.println("Failed to update attendance");
    }
}

void handleNewCard() {
    Serial.println("RFID card detected!");
    // In a real application, you would read the card ID and match it to a student
    // For this demo, we'll just pick a random student
    int randomStudent = random(Config::MAX_STUDENTS);
    updateAttendance(randomStudent);
}

// Setup and Main Loop
void setup() {
    Serial.begin(115200);
    randomSeed(analogRead(0));

    if (!initializeWiFi()) return;
    if (!initializeGoogleSheets()) return;
    
    SPI.begin();
    rfidReader.PCD_Init();
    
    initializeSpreadsheet();
    isSystemInitialized = true;
}

void loop() {
    if (!isSystemInitialized) return;

    if (!GSheet.ready()) {
        Serial.println("Lost connection to Google Sheets!");
        delay(1000);
        return;
    }

    static bool cardPresent = false;  // Track if a card is currently being processed

    // New card detected and not currently processing a card
    if (!cardPresent && rfidReader.PICC_IsNewCardPresent() && rfidReader.PICC_ReadCardSerial()) {
        cardPresent = true;  // Mark that we're processing a card
        handleNewCard();
        Serial.println("Please remove the card...");
        
        // Halt the card communication
        rfidReader.PICC_HaltA();
        rfidReader.PCD_StopCrypto1();
        
        // Wait until the card is physically removed
        delay(1000);  // Short delay to stabilize
    }
    // Check if the card has been removed
    else if (cardPresent && !rfidReader.PICC_IsNewCardPresent()) {
        cardPresent = false;  // Reset the card present flag
        Serial.println("Card removed - Ready for next scan");
        delay(500);  // Debounce delay
    }

    // Small delay in the loop
    delay(100);
}

// Student Database Implementation
const String StudentDatabase::firstNames[100] = {
    "Emma", "Liam", "Olivia", "Noah", "Ava", "Ethan", "Isabella", "Mason",
    "Sophia", "Lucas", "Mia", "Oliver", "Charlotte", "James", "Amelia", "Benjamin",
    "Evelyn", "William", "Abigail", "Henry", "Emily", "Alexander", "Elizabeth", "Michael",
    "Avery", "Daniel", "Sofia", "Matthew", "Ella", "Joseph", "Victoria", "David",
    "Grace", "Andrew", "Chloe", "Jack", "Scarlett", "Owen", "Aria", "Luke",
    "Hannah", "Sebastian", "Zoe", "Christopher", "Lily", "Julian", "Madison", "Samuel",
    "Layla", "John", "Nora", "Isaac", "Riley", "Nathan", "Hazel", "Thomas",
    "Lucy", "Charles", "Aurora", "Caleb", "Bella", "Joshua", "Anna", "Ryan",
    "Claire", "Christian", "Violet", "Carter", "Eleanor", "Jonathan", "Audrey", "Dylan",
    "Alice", "Miles", "Savannah", "Gabriel", "Maya", "Adrian", "Sarah", "Leo",
    "Eva", "Anthony", "Natalie", "Adam", "Ruby", "Nicholas", "Skylar", "Austin",
    "Naomi", "Ian", "Julia", "Robert", "Stella", "Aaron", "Katherine", "Blake",
    "Eva", "Cole", "Morgan", "Max"
};

const String StudentDatabase::lastNames[100] = {
    "Smith", "Johnson", "Williams", "Brown", "Jones", "Garcia", "Miller", "Davis",
    "Rodriguez", "Martinez", "Hernandez", "Lopez", "Gonzalez", "Wilson", "Anderson", "Thomas",
    "Taylor", "Moore", "Jackson", "Martin", "Lee", "Perez", "Thompson", "White",
    "Harris", "Sanchez", "Clark", "Ramirez", "Lewis", "Robinson", "Walker", "Young",
    "Allen", "King", "Wright", "Scott", "Torres", "Nguyen", "Hill", "Flores",
    "Green", "Adams", "Nelson", "Baker", "Hall", "Rivera", "Campbell", "Mitchell",
    "Carter", "Roberts", "Turner", "Phillips", "Evans", "Morris", "Morgan", "Cooper",
    "Peterson", "Rogers", "Reed", "Cook", "Bailey", "Bell", "Murphy", "Morgan",
    "Bennett", "Wood", "Brooks", "Kelly", "Sanders", "Price", "Barnes", "Ross",
    "Watson", "Coleman", "Jenkins", "Perry", "Powell", "Russell", "Howard", "Cox",
    "Ward", "Foster", "Gray", "Brooks", "Fisher", "Jordan", "Owen", "Reynolds",
    "Stevens", "Harrison", "Ruiz", "Kennedy", "Wells", "Burns", "Stone", "Fox",
    "Wallace", "Woods", "Cole", "West"
};