#include <freertos/FreeRTOS.h>

#include "FS.h"
#include "SPIFFS.h"

#include <ESP32Servo.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#define DATA_RESET false
#define BLE_DATA_FILE "/BLEData"
#define BLE_DEVICE_NAME     "HAHA-NO-IKARI"
#define SERVICE_UUID        "55725ac1-066c-48b5-8700-2d9fb3603c5e"
#define CHARACTERISTIC_UUID "69ddb59c-d601-4ea4-ba83-44f679a670ba"


Servo myservo;

BLEServer *gpBLEServer = NULL;
BLECharacteristic *gpCharacteristic = NULL;
bool gbBLEReady = false;
bool deviceConnected = false;
bool oldDeviceConnected = false;
char gcBLEDeviceName[ 128 ];
char gcBLEPassword[ 32 ];
char gcSettingStatus[ 32 ];
char gcStartupSettingStatus;


byte gbRXData[ 128 ];
byte gbTXData[ 128 ];
bool bleOn = false;

bool gbKeyOpen;


//---------------------------------------------------------
// Callbacks
//---------------------------------------------------------
class MyServerCallbacks: public BLEServerCallbacks {
  void onConnect(BLEServer *pServer) {
    deviceConnected = true;
    gcSettingStatus[ 0 ] = gcStartupSettingStatus;
  };
  void onDisconnect(BLEServer *pServer) {
    deviceConnected = false;
  }
};

int gbRXDataPos = 0;

class MyCharacteristicCallbacks: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic2) {
    //Serial.println("[BLE] onWrite called");
    //std::string rxValues = pCharacteristic2->getValue();
    //String rxValues = pCharacteristic2->getValue();
    //String rxValue = rxValues.c_str();
    String rxValue = pCharacteristic2->getValue();

//    Serial.println( rxValue );
//    Serial.println( rxValue.length() );

    //rxValue.trim();
    memset( gbRXData, 0x00, sizeof( gbRXData ) );
    if( rxValue.length() > 0 ){
      //bleOn = rxValue[0]!=0;
      int iOffset = gbRXDataPos;
      for(int i=0; i<rxValue.length(); i++ ){
        if ( byte( rxValue[ i ] ) == 0 ) continue;
        gbRXData[ iOffset + i ] = rxValue[ i ];
        //Serial.println( gbRXData[ iOffset + i ] );
        gbRXDataPos++;
      }
      //Serial.println( "---" );
      //Serial.println( gbRXData[ gbRXDataPos -4 ] );
      //Serial.println( gbRXData[ gbRXDataPos -3 ] );
      //Serial.println( gbRXData[ gbRXDataPos -2 ] );
      //Serial.println( gbRXData[ gbRXDataPos -1 ] );
      if ( gbRXData[ gbRXDataPos -1 ] == 0x0A ){
        gbRXData[ gbRXDataPos -1 ] = 0x00;
        bleOn = true;
        gbRXDataPos = 0;
      }
    }
  }
};


void BluetoothSetup() {
  BLEDevice::init(gcBLEDeviceName);

  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);
  gpCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ |
    BLECharacteristic::PROPERTY_WRITE |
    BLECharacteristic::PROPERTY_NOTIFY |
    BLECharacteristic::PROPERTY_INDICATE
  );
  gpCharacteristic->addDescriptor(new BLE2902());
  gpCharacteristic->setCallbacks(new MyCharacteristicCallbacks());
  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();

  // 名前だけにしてサイズを小さく
//  BLEAdvertisementData advData;
//  advData.setName("HAHA-NO-IKARI");  // ← 名前のみアドバタイズ
//  pAdvertising->setAdvertisementData(advData);

  pAdvertising->addServiceUUID(SERVICE_UUID); // UUIDはこっちで追加

  pAdvertising->setScanResponse(false); // Scan Responseは使わない
  pAdvertising->setMinPreferred(0x06);
  pAdvertising->setMinPreferred(0x12);

  BLEDevice::startAdvertising();
  Serial.println("Started advertising: HAHA-NO-IKARI");
}




void Bluetooth_Loop()
{
  int iStatus;
  
  // disconnecting
  if(!deviceConnected && oldDeviceConnected){
    vTaskDelay(500); // give the bluetooth stack the chance to get things ready
    gpBLEServer->startAdvertising();
    oldDeviceConnected = deviceConnected;
  }
  // connecting
  if(deviceConnected && !oldDeviceConnected){
    oldDeviceConnected = deviceConnected;
  }

  if ( ( deviceConnected == true ) && ( bleOn == true ) ){

    memset( gbTXData, 0x00, sizeof( gbTXData ) );
    
    iStatus = atoi( &gcSettingStatus[ 0 ] );
    switch( iStatus ){
      case 0:
        sprintf( (char *)&gbTXData, "セットするデバイス名を入力してください.\r\n" );
        gcSettingStatus[ 0 ] = '1';
      break;
      case 1:
        memmove( gcBLEDeviceName, gbRXData, sizeof( gcBLEDeviceName ) );

        sprintf( (char *)&gbTXData, "セットするパスワードを入力してください.\r\n" );
        gcSettingStatus[ 0 ] = '2';
      break;
      case 2:
        memmove( gcBLEPassword, gbRXData, sizeof( gcBLEPassword ) );
        gcSettingStatus[ 0 ] = '5';
      break;
      case 9:
        if ( 0 == strcmp( gcBLEPassword, (char *)&gbRXData[0] ) ){
          gbKeyOpen = !gbKeyOpen;

          if ( gbKeyOpen == true ){
            sprintf( (char *)&gbTXData, "Open.\r\n" );
          }else{
            sprintf( (char *)&gbTXData, "Close.\r\n" );
          }
        }else{
          sprintf( (char *)&gbTXData, "パスワードが違います.\r\n" );
        }
      break;
      default:
        sprintf( (char *)&gbTXData, "enjoy.\r\n" );
      break;
    }
    
    gpCharacteristic->setValue( (uint8_t *)&gbTXData, strlen( (char *)&gbTXData ) );
    gpCharacteristic->notify();

    vTaskDelay( 50 );

    if ( gcSettingStatus[ 0 ] == '5' ){
      gcSettingStatus[ 0 ] = '9';
      if ( SPIFFS.exists( BLE_DATA_FILE ) ) {
        SPIFFS.remove( BLE_DATA_FILE );
      }
      File f = SPIFFS.open(BLE_DATA_FILE, "w");
      if (f) {
        f.println( gcBLEDeviceName );
        f.println( gcBLEPassword );
        f.println( gcSettingStatus );
        f.close();
      }
      ESP.restart();
    }
  }
  if ( bleOn == true ) bleOn = false;

}






int correctAngle(int logicalAngle) {
  if (logicalAngle <= 90) {
    // 0〜90 → 0度はそのまま、90度で +20度補正
    return map(logicalAngle, 0, 90, 0, 70);
  } else {
    // 90〜180 → 110度から180度へ戻す
    return map(logicalAngle, 90, 180, 70, 180);
  }
}


void LoadFromSettingData(){
  if ( SPIFFS.exists( BLE_DATA_FILE ) ) {
      if ( DATA_RESET == true ) {
        Serial.println( "remove setting file." );
        SPIFFS.remove( BLE_DATA_FILE );
      }else{
        File f = SPIFFS.open(BLE_DATA_FILE, "r");
        if (f) {
          String s;
          s = f.readStringUntil('\n');
          s.trim();
          s.toCharArray( gcBLEDeviceName, sizeof( gcBLEDeviceName ) );
          s = f.readStringUntil('\n');
          s.trim();
          s.toCharArray( gcBLEPassword, sizeof( gcBLEPassword ) );
          s = f.readStringUntil('\n');
          s.trim();
          s.toCharArray( gcSettingStatus, sizeof( gcSettingStatus ) );
          
          
          f.close();
        }
      }
  }
  
  if ( (uint8_t)gcSettingStatus[ 0 ] == 0 ) {
    gcSettingStatus[ 0 ] = '0';
  }

  gcStartupSettingStatus = gcSettingStatus[ 0 ];

}

void Task_Bluetooth( void *param )
{
  BluetoothSetup();
  while( 1 )
  {
    Bluetooth_Loop();
    vTaskDelay( 1 );
  }
}


void setup() {
  Serial.begin(115200);
  myservo.setPeriodHertz(50);    // サーボ用に50Hzに設定
  myservo.attach(13, 545, 2500); // ピン13に接続、パルス幅範囲（μs）
  Serial.println("MS18 サーボテスト開始");

  gbKeyOpen = false;
  myservo.write( correctAngle( 0 ) );
  Serial.println("角度: 0度");

  memset( gbRXData, 0x00, sizeof( gbRXData ) );
  memset( gbTXData, 0x00, sizeof( gbTXData ) );
  memset( gcBLEPassword, 0x00, sizeof( gcBLEPassword ) );
  memset( gcSettingStatus, 0x00, sizeof( gcSettingStatus ) );
  strcpy(gcBLEDeviceName, BLE_DEVICE_NAME);

  if (!SPIFFS.begin()) {
    Serial.println("Formating file system");
    SPIFFS.format();
    SPIFFS.begin();
  }
  LoadFromSettingData();

  xTaskCreatePinnedToCore( Task_Bluetooth,   /* タスクの入口となる関数名 */
                           "TASK1", /* タスクの名称 */
                           4096,   /* スタックサイズ */
                           NULL,    /* パラメータのポインタ */
                           1,       /* プライオリティ */
                           NULL,    /* ハンドル構造体のポインタ */
                           0 );     /* 割り当てるコア (0/1) */


}

void fKeyOpen(){
  myservo.write( correctAngle( 90 ) );
  Serial.println("角度: 90度");

}

void fKeyClose(){
  myservo.write( correctAngle( 0 ) );
  Serial.println("角度: 0度");

}

bool gcKeyOpenEdge=false;
bool gcOldKeyOpen=false;

void loop() {
/*
  myservo.write( correctAngle( 0 ) );
  Serial.println("角度: 0度");
  delay(2000);

  myservo.write( correctAngle( 90 ) );
  Serial.println("角度: 90度");
  delay(2000);

  myservo.write( correctAngle( 180 ) );
  Serial.println("角度: 180度");
  delay(2000);
  */
  if ( gbKeyOpen == true ){
    if ( gcOldKeyOpen == false ) {
      gcKeyOpenEdge = true;
      gcOldKeyOpen = true;
    }
    if ( gcKeyOpenEdge == true ){
      gcKeyOpenEdge = false;
      fKeyOpen();
    }
  }
  if ( gbKeyOpen == false ){
    if ( gcOldKeyOpen == true ) {
      gcKeyOpenEdge = true;
      gcOldKeyOpen = false;
    }
    if ( gcKeyOpenEdge == true ){
      gcKeyOpenEdge = false;
      fKeyClose();
    }
  }


}

