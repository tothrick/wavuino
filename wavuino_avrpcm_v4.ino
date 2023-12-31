#include "mybutton.h"
#define BT1_PIN 18
#define BT2_PIN 19
#define BT3_PIN 20
#define BT4_PIN 21

MyButton btn(BT1_PIN, BT2_PIN,BT3_PIN,BT4_PIN);

short menuitem = 0; // 0 main 1 play 2 record 3 settings

String menu[]={"Play","Record","Settings"};

#include "AVRPCM.h"
#include "WAVhdr.h"
#include "SdFat.h"

#define SD_CS 4
#define SD_FAT_TYPE 1
#define SPI_CLOCK SD_SCK_MHZ(20)
#define SD_CONFIG SdSpiConfig(SD_CS, DEDICATED_SPI, SPI_CLOCK)


#if defined(__AVR_ATmega328__) || defined(__AVR_ATmega328P__)
#define DIG_OUTPIN  6
#else
#define DIG_OUTPIN  14
#endif

#define ANA_INPIN A0

#define INIT_TRY 5
#define NAME_LEN 32
#define PRINT_LEN 60

SdFat sd;
WAVhdr W;


int sdready = false;
File32 dir;
File32 file;
File32 dataFile;
char nBuf[NAME_LEN] = "12345678ABC";
char sBuf[PRINT_LEN];


//fájlokhoz könyvtáron belül
int maxentry =0;
int entry =0;
String filename;
uint16_t fileIndex;
bool dirbool = 0;
String recordfilename;
//könyvtárakhoz
int dir_arrayPointer =0;
String dirName = "/";

File32 *ff;
#define REC_SAMPLE_RATE 16000


void rgetname(){  
  int x = 0;
  dir.open(dirName.c_str()); 
 
  while(x<=entry){
    file.openNext(&dir,O_RDONLY);
    
    char f_name[100];
    file.getName(f_name,100);
    filename = f_name;
    dirbool=file.isDir();
    if(dirbool){
    filename = filename +"/";
    }
    file.close();
   
    x++;
  }
  dir.close();
}

void dirlist(){
  maxentry=0;
  
 dir.open(dirName.c_str());
 
 dir.rewind();
  while (file.openNext(&dir, O_RDONLY)) {
    maxentry = maxentry +1;
    file.close();
  }
  dir.close();
    maxentry = maxentry -1;
    entry=0;
       
}

void lcdprint(String Sor1, String Sor2){
  Serial.println(Sor1);
  Serial.println(Sor2);

  Serial.println();
}

void playit(){
  dirlist();
  rgetname();
  String sor1= "Playit";
  lcdprint (sor1,filename);

  while(1){
    int b=btn.get();
    switch (b){
      case 1:
        entry = entry-1;
        if(entry<0){
          entry = maxentry;
        }
        rgetname();
        lcdprint(sor1,filename);
      break;
  
      case 2:
        entry = entry + 1;
          if(entry > maxentry){
            entry = 0;
        }
        rgetname();
        lcdprint(sor1,filename);
      
      break;

      case 4:
        if(dirName =="/"){
          lcdprint ("Menu", menu[menuitem]);
          return;
        }
        else{
          int i = dirName.lastIndexOf('/'); //utolsó / keresése
          int j = dirName.lastIndexOf('/',i-1); //utolsó előtti
      
          if (j==0){
            dirName="/";
          }
          else{
            dirName=dirName.substring(0,j+1);
          }
      
        }
        dirlist();
        rgetname();
      lcdprint(sor1,filename);

      
      break;

      case 8:
         if(!dirbool){
            String path=dirName+filename;
            lcdprint("Fileindex: "+String(fileIndex)," String(fileIndex)");
            playfile(path.c_str());
        }
        else{
          dirName=dirName+filename;
          dirlist();
         }
     
        rgetname();
        lcdprint(sor1,filename);
     
      
      break;
  
    }
  }
}

void recordit(){
  String sor1="Recordit";
  lcdprint (sor1,"Press Select");
  while(1){
    int b=btn.get();
    switch (b){
      case 1:
            lcdprint (sor1,"+");
      break;
  
      case 2:
            lcdprint (sor1,"-");
      break;

      case 4:
      lcdprint ("Menu", menu[menuitem]);
        return;
      break;

      case 8:

        searchRecordfile();

      break;
  
    }
  }
}

void setit(){
  String sor1="Setit";
  lcdprint (sor1,"");
  while(1){
    int b=btn.get();
    switch (b){
      case 1:
               lcdprint (sor1,"+");
      break;
  
      case 2:
             lcdprint (sor1,"-");
      break;
   
      case 4:
        lcdprint ("Menu", menu[menuitem]);
        return;
      break;

      case 8:
        setdata("data1");
      break;
    }
  }
}

void playfile(String fname){
  int b;
  lcdprint("directory: "+ dirName,"");
  
  dir.open(dirName.c_str());
  dir.rewind();
  dataFile.open(&dir,fname.c_str(), O_RDONLY);
  lcdprint("Playing",filename);
  while (true) {

    if ((b = dataFile.getName(nBuf,20)) == 0)
      b = dataFile.getSFN(nBuf,20);
    
    sprintf(sBuf, "%3d %10ld %s ", dataFile.dirIndex(), dataFile.size(), nBuf);
    Serial.print(sBuf);
    wavInfo(&dataFile);
    sprintf(sBuf, "(sr:%u ch:%d bits:%d start:%lu)",
      (uint16_t)W.getData().sampleRate,
      (uint16_t)W.getData().numChannels,
      (uint16_t)W.getData().bitsPerSample,
      (uint32_t)W.getData().dataPos);
    Serial.println(sBuf);

    while (true) {
      int b = btn.get();
      if (b == 8) {
        zPlayFile(&dataFile);
        break;
      }
      if (b == 4) {
        dataFile.close();
        dir.close(); 
        lcdprint("Playit",filename);       
        return;
      }
    }
  }
}



void setdata(String dataname){

}

void zPlayFile(File32 *f) {
  uint16_t sr = 48000;
  uint32_t ds = 0, ss = 0;
  int pct;
  int pct0 = 0;

  f->rewind();
  if (f->available()) {
    if (wavInfo(f)) {
      sr = W.getData().sampleRate;
      ds = W.getData().dataSize;

      Serial.print("Setup: ");
      Serial.println(PCM_setupPWM(sr, 0));
      Serial.print("Start play: ");
      Serial.println(PCM_startPlay(true));

      while (ss < ds) {
        f->read(PCM_getPlayBuf(), PCM_BUFSIZ);
        PCM_pushPlayBuf();
        ss += PCM_BUFSIZ;
        pct = 100 * ss / ds;
        if (pct != pct0) {
          pct0 = pct;
          if (pct ==0){
            lcdprint(filename,String(pct));
          }
          //if (((pct % 10) % 3) == 1) Serial.print(".");
          if ((pct % 10) == 0) {
            lcdprint(filename,String(pct)+" %");
           
          }
        }
        if (btn.get() != 0) {
          lcdprint(filename," -break-");
          break;
        }
      }
      Serial.println();
    }
    else {
      lcdprint(filename,"Invalid file.");
    }
  }
  else
    lcdprint(filename ,"File not available.");

  lcdprint(filename,"Done.");

  lcdprint(filename,"Stop play: ");
  Serial.println(PCM_stop());
}

size_t readHandler(uint8_t *b, size_t s) {
  return ff->readBytes(b, s);
}

int wavInfo(File32 *f) {
  ff = f;
  return W.processBuffer(readHandler);
}

void searchRecordfile(){//wavuino0000.wav
 int number=0;
dir.open("/RECORDS/"); 
 
  while(file.openNext(&dir,O_RDONLY)){
       
    char f_name[20];
    file.getName(f_name,20);
    file.close();
    //Serial.println(f_name);
    String f1 = f_name;
    String f2 = f1.substring(10,11);
    String f3 = f1.substring(9,10);
    String f4 = f1.substring(8,9);
    String f5 = f1.substring(7,8);
    int i1=f2.toInt()+(f3.toInt()*10)+(f4.toInt()*100)+(f5.toInt()*1000);
    //Serial.print(f2);Serial.print("|");Serial.print(f3);Serial.print("|");Serial.print(f4);Serial.print("|");Serial.print(f5);Serial.println("|");
    
    if(i1>number){
      number = i1;
    }
    
    
  }
  dir.close();
  //Serial. println(number);
  number = number+1;
  String strnum;
  if(number <10){
    strnum = "000"+String(number);
  }
  else if(number<100){
    strnum = "00"+String(number);
  }
  else if(number<1000){
    strnum = "0"+String(number);
  }
  else{
    strnum = String(number);
  }
  recordfilename = "wavuino"+ strnum +".wav";
  
  int b;

  lcdprint("Press to record",recordfilename);
  while (btn.get() == 0);
  dir.open("/RECORDS/");
  dir.rewind();
  dataFile.open(&dir,recordfilename.c_str(), O_RDWR | O_CREAT);
  
  
  Serial.print("Recording file ");
  Serial.println(recordfilename);

  recFile(&dataFile);
  dataFile.close();
  
}

void recFile(File32 *f) {
  uint16_t sr = REC_SAMPLE_RATE;
  uint32_t ds = 0, ss = 0;

  if (f->isWritable()) {
    W.createBuffer(sr, 1, 8);
    f->write(W.getBuffer(), WAVHDR_LEN);

    Serial.print("Setup: ");
    Serial.println(PCM_setupPWM(sr, 0));
    Serial.print("Start recording: ");
    Serial.println(PCM_startRec(true));

    while (btn.get() == 0) {
      f->write(PCM_getRecBuf(), PCM_BUFSIZ);
      PCM_releaseRecBuf();
      ss += PCM_BUFSIZ;      
      if ((++ds % 100) == 0) {
        Serial.print(".");
        if (ds >= 4000) {
          Serial.println("!");
          ds = 0;
        }
      }
    }
  }
  else
    Serial.println("File not available.");
  Serial.println("Done.");

  Serial.print("Stop recording: ");
  Serial.println(PCM_stop());
  sprintf(sBuf, "%0ld bytes written.", ss);
  Serial.println(sBuf);

  W.finalizeBuffer(ss);
  f->seekSet(0);
  f->write(W.getBuffer(), WAVHDR_LEN);
  f->truncate(ss + WAVHDR_LEN);
}




void setup() {
  lcdprint ("Wavuino","V4.0");

  Serial.begin(115200);
  
  lcdprint ("Start","");

  btn.begin();

  lcdprint ("Buttons: ", String(btn.count()));

  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);
  delay(1000);

  int init_cnt = INIT_TRY;

  while (!(sdready = sd.begin(SD_CONFIG)) && init_cnt--) {
    lcdprint("#",String(init_cnt));    
    digitalWrite(SD_CS, HIGH);
    delay(500);
  }

  if (!sdready) {
    lcdprint("Card failed, or not present","");
  }
  else {
    lcdprint("card initialized.","");
  }

  PCM_init(DIG_OUTPIN, ANA_INPIN);

   if (!dir.open("/")) {
    lcdprint("Error opening root","");
    while(true);
  }

  if (!sd.exists("records")){
     lcdprint("Records folder","created");
    if(!sd.mkdir("records")){
      
      lcdprint("Cannot create","records folder");
      while(true);
    }    
  }
  else if (sd.exists("records")){
      
            lcdprint("Records OK","");
    }
  delay(1000) ; 
lcdprint ("Menu", menu[menuitem]);
  
  
}

void loop() {
  
  int b=btn.get();
  
  switch (b){
    case 1:
      menuitem = menuitem+1;
      if (menuitem > (sizeof (menu)/sizeof (menu[0]))-1){
        menuitem = 0;
        
      }
      lcdprint ("Menu", menu[menuitem]);
          
    break;
  
    case 2:
    menuitem = menuitem-1;
      if (menuitem < 0){
        menuitem = (sizeof (menu)/sizeof (menu[0]))-1;
        
      }
      lcdprint ("Menu", menu[menuitem]);
    
    break;

    
    case 4:
      Serial.print("B: ");Serial.println(b);
    break;

    case 8:

     switch (menuitem){
        case 0:
          playit();
        break;

       case 1:
        recordit();
        break;

      case 2:
        setit();
      break; 
      }
    
    break;

  }
}
