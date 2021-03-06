/* -*- c++ -*- */

/*
    Reprap firmware based on Sprinter and grbl.
 Copyright (C) 2011 Camiel Gubbels / Erik van der Zalm
 
 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 This firmware is a mashup between Sprinter and grbl.
  (https://github.com/kliment/Sprinter)
  (https://github.com/simen/grbl/tree)
 
 It has preliminary support for Matthew Roberts advance algorithm 
    http://reprap.org/pipermail/reprap-dev/2011-May/003323.html
 */

#include "Marlin.h"

#include "ultralcd.h"
#include "planner.h"
#include "stepper.h"
#include "temperature.h"
#include "motion_control.h"
#include "cardreader.h"
#include "watchdog.h"
#include "ConfigurationStore.h"
#include "language.h"
#include "pins_arduino.h"

#if DIGIPOTSS_PIN > -1
#include <SPI.h>
#endif

#define VERSION_STRING  "1.0.0"

// look here for descriptions of gcodes:http://linuxcnc.org/docs/html/gcode.html
// http://objects.reprap.org/wiki/Mendel_User_Manual:_RepRapGCodes

//Implemented Codes
//-------------------
// G0  -> G1
// G1  - Coordinated Movement X Y Z E
// G2  - CW ARC
// G3  - CCW ARC
// G4  - Dwell S<seconds> or P<milliseconds>
// G10 - retract filament according to settings of M207
// G11 - retract recover filament according to settings of M208
// G28 - Home all Axis
// G90 - Use Absolute Coordinates
// G91 - Use Relative Coordinates
// G92 - Set current position to cordinates given

//RepRap M Codes
// M0   - Unconditional stop - Wait for user to press a button on the LCD (Only if ULTRA_LCD is enabled)
// M1   - Same as M0
// M104 - Set extruder target temp
// M105 - Read current temp
// M106 - Fan on
// M107 - Fan off
// M109 - Wait for extruder current temp to reach target temp.
// M114 - Display current position

//Custom M Codes
// M17  - Enable/Power all stepper motors
// M18  - Disable all stepper motors; same as M84
// M20  - List SD card
// M21  - Init SD card
// M22  - Release SD card
// M23  - Select SD file (M23 filename.g)
// M24  - Start/resume SD print
// M25  - Pause SD print
// M26  - Set SD position in bytes (M26 S12345)
// M27  - Report SD print status
// M28  - Start SD write (M28 filename.g)
// M29  - Stop SD write
// M30  - Delete file from SD (M30 filename.g)
// M31  - Output time since last M109 or SD card start to serial
// M42  - Change pin status via gcode
// M80  - Turn on Power Supply
// M81  - Turn off Power Supply
// M82  - Set E codes absolute (default)
// M83  - Set E codes relative while in Absolute Coordinates (G90) mode
// M84  - Disable steppers until next move, 
//        or use S<seconds> to specify an inactivity timeout, after which the steppers will be disabled.  S0 to disable the timeout.
// M85  - Set inactivity shutdown timer with parameter S<seconds>. To disable set zero (default)
// M92  - Set axis_steps_per_unit - same syntax as G92
// M114 - Output current position to serial port 
// M115	- Capabilities string
// M117 - display message
// M119 - Output Endstop status to serial port
// M140 - Set bed target temp
// M190 - Wait for bed current temp to reach target temp.
// M200 - Set filament diameter
// M201 - Set max acceleration in units/s^2 for print moves (M201 X1000 Y1000)
// M202 - Set max acceleration in units/s^2 for travel moves (M202 X1000 Y1000) Unused in Marlin!!
// M203 - Set maximum feedrate that your machine can sustain (M203 X200 Y200 Z300 E10000) in mm/sec
// M204 - Set default acceleration: S normal moves T filament only moves (M204 S3000 T7000) im mm/sec^2  also sets minimum segment time in ms (B20000) to prevent buffer underruns and M20 minimum feedrate
// M205 -  advanced settings:  minimum travel speed S=while printing T=travel only,  B=minimum segment time X= maximum xy jerk, Z=maximum Z jerk, E=maximum E jerk
// M206 - set additional homeing offset
// M207 - set retract length S[positive mm] F[feedrate mm/sec] Z[additional zlift/hop]
// M208 - set recover=unretract length S[positive mm surplus to the M207 S*] F[feedrate mm/sec]
// M209 - S<1=true/0=false> enable automatic retract detect if the slicer did not support G10/11: every normal extrude-only move will be classified as retract depending on the direction.
// M220 S<factor in percent>- set speed factor override percentage
// M221 S<factor in percent>- set extrude factor override percentage
// M240 - Trigger a camera to take a photograph
// M301 - Set PID parameters P I and D
// M302 - Allow cold extrudes
// M303 - PID relay autotune S<temperature> sets the target temperature. (default target temperature = 150C)
// M304 - Set bed PID parameters P I and D
// M400 - Finish all moves
// M500 - stores paramters in EEPROM
// M501 - reads parameters from EEPROM (if you need reset them after you changed them temporarily).  
// M502 - reverts to the default "factory settings".  You still need to store them in EEPROM afterwards if you want to.
// M503 - print the current settings (from memory not from eeprom)
// M540 - Use S[0|1] to enable or disable the stop SD card print on endstop hit (requires ABORT_ON_ENDSTOP_HIT_FEATURE_ENABLED)
// M600 - Pause for filament change X[pos] Y[pos] Z[relative lift] E[initial retract] L[later retract distance for removal]
// M907 - Set digital trimpot motor current using axis codes.
// M908 - Control digital trimpot directly.
// M350 - Set microstepping mode.
// M351 - Toggle MS1 MS2 pins directly.
// M999 - Restart after being stopped by error

// ************ SCARA Specific - This can change to suit future G-code regulations
// M360 - SCARA calibration: Move to cal-position ThetaA (0 deg calibration)
// M361 - SCARA calibration: Move to cal-position ThetaB (90 deg calibration - steps per degree)
// M362 - SCARA calibration: Move to cal-position PsiA (0 deg calibration)
// M363 - SCARA calibration: Move to cal-position PsiB (90 deg calibration - steps per degree)
// M364 - SCARA calibration: Move to cal-position PSIC (90 deg to Theta calibration position)
// M365 - SCARA calibration: Scaling factor, X, Y, Z axis
// M366 - SCARA calibration: Move to Theta and Psi position

// M370 - Morgan Z calibration: Initialise calbration - Specifying N will cause the init not to clear out the current grid and allow for fine tuning
// M371 - Manual claibration point program
// M372 - Calculate all calibration points using data aquired
// M373 - End calibration
// M375 - Dsiplay calibration matrix

//Stepper Movement Variables

//===========================================================================
//=============================imported variables============================
//===========================================================================


//===========================================================================
//=============================public variables=============================
//===========================================================================
#ifdef SDSUPPORT
CardReader card;
#endif
float homing_feedrate[] = HOMING_FEEDRATE;
bool axis_relative_modes[] = AXIS_RELATIVE_MODES;
int feedmultiply=100; //100->1 200->2
int saved_feedmultiply;
int extrudemultiply=100; //100->1 200->2
float current_position[NUM_AXIS] = { 0.0, 0.0, 0.0, 0.0 };
float add_homeing[NUM_AXIS]={0,0,0,0};                            // Additional Homing :Theta and Psi is X and Y for SCARA
float axis_scaling[NUM_AXIS]={1,1,1,1};                           // Build size scaling

bool SoftEndsEnabled = true;

float Arm_lookup[X_ARMLOOKUP_LENGTH][Y_ARMLOOKUP_LENGTH];
bool Y_gridcal = false;                                        // Normal mode on reset.

int GCal_X = 3, GCal_Y = 3,  // Position points for GridCal (Default 3) 
           GPos_X = 0, GPos_Y = 0;  // used to keep calibration positions in loop


float min_pos[3] = { X_MIN_POS, Y_MIN_POS, Z_MIN_POS };
float max_pos[3] = { X_MAX_POS, Y_MAX_POS, Z_MAX_POS };
uint8_t active_extruder = 0;
int fanSpeed=0;

#ifdef FWRETRACT
  bool autoretract_enabled=true;
  bool retracted=false;
  float retract_length=3, retract_feedrate=17*60, retract_zlift=0.8;
  float retract_recover_length=0, retract_recover_feedrate=8*60;
#endif

//===========================================================================
//=============================private variables=============================
//===========================================================================
const char axis_codes[NUM_AXIS] = {'X', 'Y', 'Z', 'E'};
static float destination[NUM_AXIS] = {  0.0, 0.0, 0.0, 0.0};
static float delta[3] = {0.0, 0.0, 0.0};
//static float homingoffset[3] = {0.0 , 0.0, 0.0};
static float offset[3] = {0.0, 0.0, 0.0};
static bool home_all_axis = true;
static bool home_XY = true;
static float feedrate = 1500.0, next_feedrate, saved_feedrate;
static long gcode_N, gcode_LastN, Stopped_gcode_LastN = 0;

static float SCARA_C2, SCARA_S2, SCARA_K1, SCARA_K2, SCARA_theta, SCARA_psi, dCal_X = 0, dCal_Y = 0, dCal_X_Inv = 0, dCal_Y_Inv = 0;
//static float Grid_temp[X_ARMLOOKUP_LENGTH][Y_ARMLOOKUP_LENGTH];

static bool relative_mode = false;  //Determines Absolute or Relative Coordinates

static char cmdbuffer[BUFSIZE][MAX_CMD_SIZE];
static bool fromsd[BUFSIZE];
static int bufindr = 0;
static int bufindw = 0;
static int buflen = 0;
//static int i = 0;
static char serial_char;
static int serial_count = 0;
static boolean comment_mode = false;
static char *strchr_pointer; // just a pointer to find chars in the cmd string like X, Y, Z, E, etc

const int sensitive_pins[] = SENSITIVE_PINS; // Sensitive pin list for M42

//static float tt = 0;
//static float bt = 0;

//Inactivity shutdown variables
static unsigned long previous_millis_cmd = 0;
static unsigned long max_inactive_time = 0;
static unsigned long stepper_inactive_time = DEFAULT_STEPPER_DEACTIVE_TIME*1000l;

unsigned long starttime=0;
unsigned long stoptime=0;

static uint8_t tmp_extruder;


bool Stopped=false;

//===========================================================================
//=============================ROUTINES=============================
//===========================================================================

void get_arc_coordinates();
bool setTargetedHotend(int code);

void serial_echopair_P(const char *s_P, float v)
    { serialprintPGM(s_P); SERIAL_ECHO(v); }
void serial_echopair_P(const char *s_P, double v)
    { serialprintPGM(s_P); SERIAL_ECHO(v); }
void serial_echopair_P(const char *s_P, unsigned long v)
    { serialprintPGM(s_P); SERIAL_ECHO(v); }

extern "C"{
  extern unsigned int __bss_end;
  extern unsigned int __heap_start;
  extern void *__brkval;

  int freeMemory() {
    int free_memory;

    if((int)__brkval == 0)
      free_memory = ((int)&free_memory) - ((int)&__bss_end);
    else
      free_memory = ((int)&free_memory) - ((int)__brkval);

    return free_memory;
  }
}

//adds an command to the main command buffer
//thats really done in a non-safe way.
//needs overworking someday
void enquecommand(const char *cmd)
{
  if(buflen < BUFSIZE)
  {
    //this is dangerous if a mixing of serial and this happsens
    strcpy(&(cmdbuffer[bufindw][0]),cmd);
    SERIAL_ECHO_START;
    SERIAL_ECHOPGM("enqueing \"");
    SERIAL_ECHO(cmdbuffer[bufindw]);
    SERIAL_ECHOLNPGM("\"");
    bufindw= (bufindw + 1)%BUFSIZE;
    buflen += 1;
  }
}

void enquecommand_P(const char *cmd)
{
  if(buflen < BUFSIZE)
  {
    //this is dangerous if a mixing of serial and this happsens
    strcpy_P(&(cmdbuffer[bufindw][0]),cmd);
    SERIAL_ECHO_START;
    SERIAL_ECHOPGM("enqueing \"");
    SERIAL_ECHO(cmdbuffer[bufindw]);
    SERIAL_ECHOLNPGM("\"");
    bufindw= (bufindw + 1)%BUFSIZE;
    buflen += 1;
  }
}

void setup_killpin()
{
  #if( KILL_PIN>-1 )
    pinMode(KILL_PIN,INPUT);
    WRITE(KILL_PIN,HIGH);
  #endif
}
    
void setup_photpin()
{
  #ifdef PHOTOGRAPH_PIN
    #if (PHOTOGRAPH_PIN > -1)
    SET_OUTPUT(PHOTOGRAPH_PIN);
    WRITE(PHOTOGRAPH_PIN, LOW);
    #endif
  #endif 
}

void setup_powerhold()
{
 #ifdef SUICIDE_PIN
   #if (SUICIDE_PIN> -1)
      SET_OUTPUT(SUICIDE_PIN);
      WRITE(SUICIDE_PIN, HIGH);
   #endif
 #endif
 #if (PS_ON_PIN > -1)
   SET_OUTPUT(PS_ON_PIN);
   WRITE(PS_ON_PIN, PS_ON_AWAKE);
 #endif
}

void suicide()
{
 #ifdef SUICIDE_PIN
    #if (SUICIDE_PIN> -1) 
      SET_OUTPUT(SUICIDE_PIN);
      WRITE(SUICIDE_PIN, LOW);
    #endif
  #endif
}

void setup()
{
  setup_killpin(); 
  setup_powerhold();
  MYSERIAL.begin(BAUDRATE);
  SERIAL_PROTOCOLLNPGM("start");
  SERIAL_ECHO_START;

  // Check startup - does nothing if bootloader sets MCUSR to 0
  byte mcu = MCUSR;
  if(mcu & 1) SERIAL_ECHOLNPGM(MSG_POWERUP);
  if(mcu & 2) SERIAL_ECHOLNPGM(MSG_EXTERNAL_RESET);
  if(mcu & 4) SERIAL_ECHOLNPGM(MSG_BROWNOUT_RESET);
  if(mcu & 8) SERIAL_ECHOLNPGM(MSG_WATCHDOG_RESET);
  if(mcu & 32) SERIAL_ECHOLNPGM(MSG_SOFTWARE_RESET);
  MCUSR=0;

  SERIAL_ECHOPGM(MSG_MARLIN);
  SERIAL_ECHOLNPGM(VERSION_STRING);
  #ifdef STRING_VERSION_CONFIG_H
    #ifdef STRING_CONFIG_H_AUTHOR
      SERIAL_ECHO_START;
      SERIAL_ECHOPGM(MSG_CONFIGURATION_VER);
      SERIAL_ECHOPGM(STRING_VERSION_CONFIG_H);
      SERIAL_ECHOPGM(MSG_AUTHOR);
      SERIAL_ECHOLNPGM(STRING_CONFIG_H_AUTHOR);
      SERIAL_ECHOPGM("Compiled: ");
      SERIAL_ECHOLNPGM(__DATE__);
    #endif
  #endif
  SERIAL_ECHO_START;
  SERIAL_ECHOPGM(MSG_FREE_MEMORY);
  SERIAL_ECHO(freeMemory());
  SERIAL_ECHOPGM(MSG_PLANNER_BUFFER_BYTES);
  SERIAL_ECHOLN((int)sizeof(block_t)*BLOCK_BUFFER_SIZE);
  for(int8_t i = 0; i < BUFSIZE; i++)
  {
    fromsd[i] = false;
  }
  
  Config_RetrieveSettings(); // loads data from EEPROM if available

  for(int8_t i=0; i < NUM_AXIS; i++)
  {
    axis_steps_per_sqr_second[i] = max_acceleration_units_per_sq_second[i] * axis_steps_per_unit[i];
  }


  tp_init();    // Initialize temperature loop 
  plan_init();  // Initialize planner;
  watchdog_init();
  st_init();    // Initialize stepper, this enables interrupts!
  setup_photpin();
  
  lcd_init();
}


void loop()
{
  if(buflen < (BUFSIZE-1))
    get_command();
  #ifdef SDSUPPORT
  card.checkautostart(false);
  #endif
  if(buflen)
  {
    #ifdef SDSUPPORT
      if(card.saving)
      {
	if(strstr_P(cmdbuffer[bufindr], PSTR("M29")) == NULL)
	{
	  card.write_command(cmdbuffer[bufindr]);
	  SERIAL_PROTOCOLLNPGM(MSG_OK);
	}
	else
	{
	  card.closefile();
	  SERIAL_PROTOCOLLNPGM(MSG_FILE_SAVED);
	}
      }
      else
      {
	process_commands();
      }
    #else
      process_commands();
    #endif //SDSUPPORT
    buflen = (buflen-1);
    bufindr = (bufindr + 1)%BUFSIZE;
  }
  //check heater every n milliseconds
  manage_heater();
  manage_inactivity();
  checkHitEndstops();
  lcd_update();
}

void get_command() 
{ 
  while( MYSERIAL.available() > 0  && buflen < BUFSIZE) {
    serial_char = MYSERIAL.read();
    if(serial_char == '\n' || 
       serial_char == '\r' || 
       (serial_char == ':' && comment_mode == false) || 
       serial_count >= (MAX_CMD_SIZE - 1) ) 
    {
      if(!serial_count) { //if empty line
        comment_mode = false; //for new command
        return;
      }
      cmdbuffer[bufindw][serial_count] = 0; //terminate string
      if(!comment_mode){
        comment_mode = false; //for new command
        fromsd[bufindw] = false;
        if(strchr(cmdbuffer[bufindw], 'N') != NULL)
        {
          strchr_pointer = strchr(cmdbuffer[bufindw], 'N');
          gcode_N = (strtol(&cmdbuffer[bufindw][strchr_pointer - cmdbuffer[bufindw] + 1], NULL, 10));
          if(gcode_N != gcode_LastN+1 && (strstr_P(cmdbuffer[bufindw], PSTR("M110")) == NULL) ) {
            SERIAL_ERROR_START;
            SERIAL_ERRORPGM(MSG_ERR_LINE_NO);
            SERIAL_ERRORLN(gcode_LastN);
            //Serial.println(gcode_N);
            FlushSerialRequestResend();
            serial_count = 0;
            return;
          }

          if(strchr(cmdbuffer[bufindw], '*') != NULL)
          {
            byte checksum = 0;
            byte count = 0;
            while(cmdbuffer[bufindw][count] != '*') checksum = checksum^cmdbuffer[bufindw][count++];
            strchr_pointer = strchr(cmdbuffer[bufindw], '*');

            if( (int)(strtod(&cmdbuffer[bufindw][strchr_pointer - cmdbuffer[bufindw] + 1], NULL)) != checksum) {
              SERIAL_ERROR_START;
              SERIAL_ERRORPGM(MSG_ERR_CHECKSUM_MISMATCH);
              SERIAL_ERRORLN(gcode_LastN);
              FlushSerialRequestResend();
              serial_count = 0;
              return;
            }
            //if no errors, continue parsing
          }
          else 
          {
            SERIAL_ERROR_START;
            SERIAL_ERRORPGM(MSG_ERR_NO_CHECKSUM);
            SERIAL_ERRORLN(gcode_LastN);
            FlushSerialRequestResend();
            serial_count = 0;
            return;
          }

          gcode_LastN = gcode_N;
          //if no errors, continue parsing
        }
        else  // if we don't receive 'N' but still see '*'
        {
          if((strchr(cmdbuffer[bufindw], '*') != NULL))
          {
            SERIAL_ERROR_START;
            SERIAL_ERRORPGM(MSG_ERR_NO_LINENUMBER_WITH_CHECKSUM);
            SERIAL_ERRORLN(gcode_LastN);
            serial_count = 0;
            return;
          }
        }
        if((strchr(cmdbuffer[bufindw], 'G') != NULL)){
          strchr_pointer = strchr(cmdbuffer[bufindw], 'G');
          switch((int)((strtod(&cmdbuffer[bufindw][strchr_pointer - cmdbuffer[bufindw] + 1], NULL)))){
          case 0:
          case 1:
          case 2:
          case 3:
            if(Stopped == false) { // If printer is stopped by an error the G[0-3] codes are ignored.
	      #ifdef SDSUPPORT
              if(card.saving)
                break;
	      #endif //SDSUPPORT
              SERIAL_PROTOCOLLNPGM(MSG_OK); 
            }
            else {
              SERIAL_ERRORLNPGM(MSG_ERR_STOPPED);
              LCD_MESSAGEPGM(MSG_STOPPED);
            }
            break;
          default:
            break;
          }

        }
        bufindw = (bufindw + 1)%BUFSIZE;
        buflen += 1;
      }
      serial_count = 0; //clear buffer
    }
    else
    {
      if(serial_char == ';') comment_mode = true;
      if(!comment_mode) cmdbuffer[bufindw][serial_count++] = serial_char;
    }
  }
  #ifdef SDSUPPORT
  if(!card.sdprinting || serial_count!=0){
    return;
  }
  while( !card.eof()  && buflen < BUFSIZE) {
    int16_t n=card.get();
    serial_char = (char)n;
    if(serial_char == '\n' || 
       serial_char == '\r' || 
       (serial_char == ':' && comment_mode == false) || 
       serial_count >= (MAX_CMD_SIZE - 1)||n==-1) 
    {
      if(card.eof()){
        SERIAL_PROTOCOLLNPGM(MSG_FILE_PRINTED);
        stoptime=millis();
        char time[30];
        unsigned long t=(stoptime-starttime)/1000;
        int hours, minutes;
        minutes=(t/60)%60;
        hours=t/60/60;
        sprintf_P(time, PSTR("%i hours %i minutes"),hours, minutes);
        SERIAL_ECHO_START;
        SERIAL_ECHOLN(time);
        lcd_setstatus(time);
        card.printingHasFinished();
        card.checkautostart(true);
        
      }
      if(!serial_count)
      {
        comment_mode = false; //for new command
        return; //if empty line
      }
      cmdbuffer[bufindw][serial_count] = 0; //terminate string
//      if(!comment_mode){
        fromsd[bufindw] = true;
        buflen += 1;
        bufindw = (bufindw + 1)%BUFSIZE;
//      }     
      comment_mode = false; //for new command
      serial_count = 0; //clear buffer
    }
    else
    {
      if(serial_char == ';') comment_mode = true;
      if(!comment_mode) cmdbuffer[bufindw][serial_count++] = serial_char;
    }
  }
  
  #endif //SDSUPPORT

}


float code_value() 
{ 
  return (strtod(&cmdbuffer[bufindr][strchr_pointer - cmdbuffer[bufindr] + 1], NULL)); 
}

long code_value_long() 
{ 
  return (strtol(&cmdbuffer[bufindr][strchr_pointer - cmdbuffer[bufindr] + 1], NULL, 10)); 
}

bool code_seen(char code)
{
  strchr_pointer = strchr(cmdbuffer[bufindr], code);
  return (strchr_pointer != NULL);  //Return True if a character was found
}

#define DEFINE_PGM_READ_ANY(type, reader)		\
    static inline type pgm_read_any(const type *p)	\
	{ return pgm_read_##reader##_near(p); }

DEFINE_PGM_READ_ANY(float,       float);
DEFINE_PGM_READ_ANY(signed char, byte);

#define XYZ_CONSTS_FROM_CONFIG(type, array, CONFIG)	\
static const PROGMEM type array##_P[3] =		\
    { X_##CONFIG, Y_##CONFIG, Z_##CONFIG };		\
static inline type array(int axis)			\
    { return pgm_read_any(&array##_P[axis]); }

XYZ_CONSTS_FROM_CONFIG(float, base_min_pos,    MIN_POS);
XYZ_CONSTS_FROM_CONFIG(float, base_max_pos,    MAX_POS);
XYZ_CONSTS_FROM_CONFIG(float, base_home_pos,   HOME_POS);
XYZ_CONSTS_FROM_CONFIG(float, max_length,      MAX_LENGTH);
XYZ_CONSTS_FROM_CONFIG(float, home_retract_mm, HOME_RETRACT_MM);
XYZ_CONSTS_FROM_CONFIG(signed char, home_dir,  HOME_DIR);

static void axis_is_at_home(int axis) {
 #ifdef MORGAN_SCARA
   float homeposition[3];
   char i;
   
   if (axis < 2)
   {
   
     for (i=0; i<3; i++)
     {
        homeposition[i] = base_home_pos(i); 
     }  
 
   // Works out real Homeposition angles using inverse kinematics, 
   // and calculates homing offset using forward kinematics
     calculate_delta(homeposition);
     
     //SERIAL_ECHOPGM("base Theta="); SERIAL_ECHO(delta[X_AXIS]);
     //SERIAL_ECHOPGM("   base Psi+Theta="); SERIAL_ECHOLN(delta[Y_AXIS]);
     
     //SERIAL_ECHOPGM("addhome X="); SERIAL_ECHO(add_homeing[X_AXIS]);
     //SERIAL_ECHOPGM("addhome Theta="); SERIAL_ECHO(delta[X_AXIS]);
     //SERIAL_ECHOPGM("   addhome Psi+Theta="); SERIAL_ECHOLN(delta[Y_AXIS]);
     
      delta[X_AXIS] = 90;      // Theta
      delta[Y_AXIS] = 180;     // Psi

     calculate_forward(delta);
     
     //SERIAL_ECHOPGM("addhome X="); SERIAL_ECHO(delta[X_AXIS]);
     //SERIAL_ECHOPGM("   addhome Y="); SERIAL_ECHOLN(delta[Y_AXIS]);

      current_position[axis] = delta[axis];
    
    // SCARA home positions are based on configuration since the actual limits are determined by the 
    // inverse kinematic transform.
    min_pos[axis] =          base_min_pos(axis); // + (delta[axis] - base_home_pos(axis));
    max_pos[axis] =          base_max_pos(axis); // + (delta[axis] - base_home_pos(axis));
   } 
   else
   {
 
      current_position[axis] = base_home_pos(axis) + add_homeing[axis];
      min_pos[axis] =          base_min_pos(axis) + add_homeing[axis];
      max_pos[axis] =          base_max_pos(axis) + add_homeing[axis];

     
   }  
 
 
 #else
 
  current_position[axis] = base_home_pos(axis) + add_homeing[axis];
  min_pos[axis] =          base_min_pos(axis) + add_homeing[axis];
  max_pos[axis] =          base_max_pos(axis) + add_homeing[axis];
 #endif 
}

static void homeaxis(int axis) {
#define HOMEAXIS_DO(LETTER) \
  ((LETTER##_MIN_PIN > -1 && LETTER##_HOME_DIR==-1) || (LETTER##_MAX_PIN > -1 && LETTER##_HOME_DIR==1))

  if (axis==X_AXIS ? HOMEAXIS_DO(X) :
      axis==Y_AXIS ? HOMEAXIS_DO(Y) :
      axis==Z_AXIS ? HOMEAXIS_DO(Z) :
      0) {
    current_position[axis] = 0;
    plan_set_position(current_position[X_AXIS], current_position[Y_AXIS], current_position[Z_AXIS], current_position[E_AXIS]);
    destination[axis] = 3 * Z_MAX_LENGTH * home_dir(axis);
    feedrate = homing_feedrate[axis];
    plan_buffer_line(destination[X_AXIS], destination[Y_AXIS], destination[Z_AXIS], destination[E_AXIS], feedrate/60, active_extruder);
    st_synchronize();
   
    current_position[axis] = 0;
    plan_set_position(current_position[X_AXIS], current_position[Y_AXIS], current_position[Z_AXIS], current_position[E_AXIS]);
    destination[axis] = -home_retract_mm(axis) * home_dir(axis);
    plan_buffer_line(destination[X_AXIS], destination[Y_AXIS], destination[Z_AXIS], destination[E_AXIS], feedrate/60, active_extruder);
    st_synchronize();
   
    destination[axis] = 2*home_retract_mm(axis) * home_dir(axis);
    feedrate = homing_feedrate[axis]/2 ; 
    plan_buffer_line(destination[X_AXIS], destination[Y_AXIS], destination[Z_AXIS], destination[E_AXIS], feedrate/60, active_extruder);
    st_synchronize();
   
    axis_is_at_home(axis);					
    destination[axis] = current_position[axis];
    feedrate = 0.0;
    endstops_hit_on_purpose();
  }

#ifdef MORGAN_USE_Y_ENDSTOPS_FOR_HOME_AND_CALIBRATE  
  // As part of Morgan Psi Calibration, move it back to MAX Y ENDSTOP
  if (axis==Y_AXIS)
  {
    current_position[axis] = 0;
    plan_set_position(current_position[X_AXIS], current_position[Y_AXIS], current_position[Z_AXIS], current_position[E_AXIS]);
    destination[axis] = 3 * Z_MAX_LENGTH * -home_dir(axis);
    feedrate = homing_feedrate[axis];
    plan_buffer_line(destination[X_AXIS], destination[Y_AXIS], destination[Z_AXIS], destination[E_AXIS], feedrate/60, active_extruder);
    st_synchronize();

    axis_is_at_home(axis);					
    destination[axis] = current_position[axis];
    feedrate = 0.0;
    endstops_hit_on_purpose();
  }
#endif

}
#define HOMEAXIS(LETTER) homeaxis(LETTER##_AXIS)

// Physcially moves the extruder on the given axis by incrementing with the given delta
void advance_axis (int axis, float delta_to_add)
{
    current_position[axis] = 0;
    plan_set_position(current_position[X_AXIS], current_position[Y_AXIS], current_position[Z_AXIS], current_position[E_AXIS]);
    destination[axis] = -delta_to_add * home_dir(axis);
    plan_buffer_line(destination[X_AXIS], destination[Y_AXIS], destination[Z_AXIS], destination[E_AXIS], feedrate/60, active_extruder);
    st_synchronize();
}

void HomeAllAxis()
{
      // Get the head away from the bed/printed object before we zero to home
      advance_axis(Z_AXIS,5);
  
      saved_feedrate = feedrate;
      saved_feedmultiply = feedmultiply;
      feedmultiply = 100;
      previous_millis_cmd = millis();
      
      dCal_X = X_MAX_POS/GCal_X;                   // Initialise dCal_X & dCal_Y
      dCal_X_Inv = 1.0/dCal_X;
      dCal_Y = Y_MAX_POS/GCal_Y;
      dCal_Y_Inv = 1.0/dCal_Y;
      
      enable_endstops(true);
      
      for(int8_t i=0; i < NUM_AXIS; i++) {
        destination[i] = current_position[i];
      }
      feedrate = 0.0;
      home_XY = ((code_seen(axis_codes[0])) || (code_seen(axis_codes[1])));  // Always home XY together...
     
      home_all_axis = !((code_seen(axis_codes[0])) || (code_seen(axis_codes[1])) || (code_seen(axis_codes[2])))
                    || ((code_seen(axis_codes[0])) && (code_seen(axis_codes[1])) && (code_seen(axis_codes[2])));
      
      #ifdef QUICK_HOME
        if (home_all_axis || home_XY)  // Move all carriages up together until the first endstop is hit.
        {
        current_position[X_AXIS] = 0;
        current_position[Y_AXIS] = 0;
        current_position[Z_AXIS] = 0;
        plan_set_position(current_position[X_AXIS], current_position[Y_AXIS], current_position[Z_AXIS], current_position[E_AXIS]); 

        destination[X_AXIS] = 3 * Z_MAX_LENGTH;
        destination[Y_AXIS] = 3 * Z_MAX_LENGTH;
        destination[Z_AXIS] = current_position[Z_AXIS];
        feedrate = homing_feedrate[X_AXIS];
        plan_buffer_line(destination[X_AXIS], destination[Y_AXIS], destination[Z_AXIS], destination[E_AXIS], feedrate/60, active_extruder);
        st_synchronize();
        endstops_hit_on_purpose();

        current_position[X_AXIS] = destination[X_AXIS];
        current_position[Y_AXIS] = destination[Y_AXIS];
        //current_position[Z_AXIS] = destination[Z_AXIS];
      }
      
      #endif
      
      // Not a good idea for SCARA robots to home the arms individually... Make sure QUICK_HOME is enabled for Morgan!
      
      if((home_all_axis || home_XY) || (code_seen(axis_codes[X_AXIS]))) 
      {
        HOMEAXIS(X);
      }

      if((home_all_axis || home_XY) || (code_seen(axis_codes[Y_AXIS]))) {
        HOMEAXIS(Y);
      }
      
      if((home_all_axis) || (code_seen(axis_codes[Z_AXIS]))) {
        HOMEAXIS(Z);
      }
      
      if(code_seen(axis_codes[X_AXIS])) 
      {
        if(code_value_long() != 0) {
          current_position[X_AXIS]=code_value();// +add_homeing[0];
        }
      }

      if(code_seen(axis_codes[Y_AXIS])) {
        if(code_value_long() != 0) {
          current_position[Y_AXIS]=code_value();// +add_homeing[1];
        }
      }

      if(code_seen(axis_codes[Z_AXIS])) {
        if(code_value_long() != 0) {
          current_position[Z_AXIS]=code_value()+add_homeing[2];
        }
      }
      calculate_delta(current_position);
      plan_set_position(delta[X_AXIS], delta[Y_AXIS], delta[Z_AXIS], current_position[E_AXIS]);
      
      #ifdef ENDSTOPS_ONLY_FOR_HOMING
        enable_endstops(false);
      #endif
      
      feedrate = saved_feedrate;
      feedmultiply = saved_feedmultiply;
      previous_millis_cmd = millis();
      endstops_hit_on_purpose();
      
      // Move to a safe starting position. Using physical switches as endstops on 
      // the Morgan wheel prevents the extruder from moving in a certain area infront 
      // of the main axes assembly. Move the head to a position well away from this 
      // deadzone or run the risk of bashing the switches trying to reach an interim 
      // position
      #if defined(SCARA_home_safe_starting_x) && defined(SCARA_home_safe_starting_y)
        feedmultiply = 100;
        feedrate = homing_feedrate[X_AXIS];;

        destination[Z_AXIS] = current_position[Z_AXIS] + 5;
//        destination[X_AXIS] = SCARA_home_safe_starting_x;
//        destination[Y_AXIS] = SCARA_home_safe_starting_y;
        destination[X_AXIS] = current_position[X_AXIS];
        destination[Y_AXIS] = current_position[Y_AXIS];
           
        prepare_move();
        st_synchronize();    // finish move

        feedrate = saved_feedrate;
        feedmultiply = saved_feedmultiply;

        SERIAL_ECHO_START;
        SERIAL_ECHOLN("Homed at position :");
        SERIAL_ECHO(" X:");
        SERIAL_ECHO(current_position[X_AXIS]);
        SERIAL_ECHO(" Y:");
        SERIAL_ECHO(current_position[Y_AXIS]);
        SERIAL_ECHO(" Z:");
        SERIAL_ECHOLN(current_position[Z_AXIS]);
      #endif
}

// Moves the head to the expected X and Y coordinate with the Z as the last saved calibration value for that position.
void ZCalMoveIntoToPosition()
{
    // Move to the next position and try not to catch the bed by lowering it first and then lifting it
    destination[Z_AXIS] = 5;
    feedrate = 10000;
    
    prepare_move();
    st_synchronize();    // finish move
    
    destination[X_AXIS] = GPos_X * X_MAX_POS / GCal_X + 1;  // Add one to make sure the right cal grid item is indexed in the lookup later
    destination[Y_AXIS] = GPos_Y * Y_MAX_POS / GCal_X + 1;  // Add one to make sure the right cal grid item is indexed in the lookup later
      
    prepare_move();
    st_synchronize();    // finish move
    
    // Prepare for the calibration. This will be 0 if the grid was cleared by M370 C otherwise we start with the previous calibration session value
    destination[Z_AXIS] = Arm_lookup[(int)(current_position[X_AXIS]/X_MAX_POS * GCal_X)][(int)(current_position[Y_AXIS]/Y_MAX_POS * GCal_Y)];
    
    prepare_move();
    st_synchronize();    // finish move
    
    SERIAL_ECHO_START;
    SERIAL_ECHOLN("Moved to lookup position :");
    SERIAL_ECHO(" X:");
    SERIAL_ECHO(current_position[X_AXIS]);
    SERIAL_ECHO(" Y:");
    SERIAL_ECHO(current_position[Y_AXIS]);
    SERIAL_ECHO(" Z:");
    SERIAL_ECHOLN(current_position[Z_AXIS]);
}

void process_commands()
{
  unsigned long codenum; //throw away variable
  char *starpos = NULL;
  
  int counterx, countery;

  if(code_seen('G'))
  {
    switch((int)code_value())
    {
    case 0: // G0 -> G1
    case 1: // G1
      if(Stopped == false && dCal_X) {    // Ensure Unit homed.
        //  SERIAL_ECHOPGM("cartesian x="); SERIAL_ECHO(destination[X_AXIS]);
        //  SERIAL_ECHOPGM(" y="); SERIAL_ECHOLN(destination[Y_AXIS]);
        get_coordinates(true); // For X Y Z E F
        //   SERIAL_ECHOPGM("cartesian x="); SERIAL_ECHO(destination[X_AXIS]);
        //  SERIAL_ECHOPGM(" y="); SERIAL_ECHOLN(destination[Y_AXIS]);
        prepare_move();
        //   SERIAL_ECHOPGM("cartesian x="); SERIAL_ECHO(destination[X_AXIS]);
        //  SERIAL_ECHOPGM(" y="); SERIAL_ECHOLN(destination[Y_AXIS]);
        //ClearToSend();
        return;
      }
      else {
         SERIAL_ECHOLN("  No movement - Home first...");
      }  
      break;
    /*      // Disable Arcs, for now.
    case 2: // G2  - CW ARC
      if(Stopped == false) {
        get_arc_coordinates();
        prepare_arc_move(true);
        return;
      }
    case 3: // G3  - CCW ARC
      if(Stopped == false) {
        get_arc_coordinates();
        prepare_arc_move(false);
        return;
      } */
    case 4: // G4 dwell
      LCD_MESSAGEPGM(MSG_DWELL);
      codenum = 0;
      if(code_seen('P')) codenum = code_value(); // milliseconds to wait
      if(code_seen('S')) codenum = code_value() * 1000; // seconds to wait
      
      st_synchronize();
      codenum += millis();  // keep track of when we started waiting
      previous_millis_cmd = millis();
      while(millis()  < codenum ){
        manage_heater();
        manage_inactivity();
        lcd_update();
      }
      break;
      #ifdef FWRETRACT  
      case 10: // G10 retract
      if(!retracted) 
      {
        destination[X_AXIS]=current_position[X_AXIS];
        destination[Y_AXIS]=current_position[Y_AXIS];
        destination[Z_AXIS]=current_position[Z_AXIS]; 
        current_position[Z_AXIS]+=-retract_zlift;
        destination[E_AXIS]=current_position[E_AXIS]-retract_length; 
        feedrate=retract_feedrate;
        retracted=true;
        prepare_move();
      }
      
      break;
      case 11: // G10 retract_recover
      if(!retracted) 
      {
        destination[X_AXIS]=current_position[X_AXIS];
        destination[Y_AXIS]=current_position[Y_AXIS];
        destination[Z_AXIS]=current_position[Z_AXIS]; 
        
        current_position[Z_AXIS]+=retract_zlift;
        current_position[E_AXIS]+=-retract_recover_length; 
        feedrate=retract_recover_feedrate;
        retracted=false;
        prepare_move();
      }
      break;
      #endif //FWRETRACT
    case 28: //G28 Home all Axis one at a time 

      HomeAllAxis();

      break;
    case 90: // G90
      relative_mode = false;
      break;
    case 91: // G91
      relative_mode = true;
      break;
    case 92: // G92
      if(!code_seen(axis_codes[E_AXIS]))
        st_synchronize();
      for(int8_t i=0; i < NUM_AXIS; i++) {
        if(code_seen(axis_codes[i])) { 
           if(i == E_AXIS) {
             current_position[i] = code_value();  
             plan_set_e_position(current_position[E_AXIS]);
           }
           else {
             if (i == X_AXIS || i == Y_AXIS) {
                 current_position[i] = code_value();  
             }
             else {
                 current_position[i] = code_value()+add_homeing[i];  
             }    
             plan_set_position(current_position[X_AXIS], current_position[Y_AXIS], current_position[Z_AXIS], current_position[E_AXIS]);
           }
        }
      }
      break;
    }
  }

  else if(code_seen('M'))
  {
    switch( (int)code_value() ) 
    {
#ifdef ULTIPANEL
    case 0: // M0 - Unconditional stop - Wait for user button press on LCD
    case 1: // M1 - Conditional stop - Wait for user button press on LCD
    {
      LCD_MESSAGEPGM(MSG_USERWAIT);
      codenum = 0;
      if(code_seen('P')) codenum = code_value(); // milliseconds to wait
      if(code_seen('S')) codenum = code_value() * 1000; // seconds to wait
      
      st_synchronize();
      previous_millis_cmd = millis();
      if (codenum > 0){
        codenum += millis();  // keep track of when we started waiting
        while(millis()  < codenum && !LCD_CLICKED){
          manage_heater();
          manage_inactivity();
          lcd_update();
        }
      }else{
        while(!LCD_CLICKED){
          manage_heater();
          manage_inactivity();
          lcd_update();
        }
      }
      LCD_MESSAGEPGM(MSG_RESUMING);
    }
    break;
#endif
    case 17:
        LCD_MESSAGEPGM(MSG_NO_MOVE);
        enable_x(); 
        enable_y(); 
        enable_z(); 
        enable_e0(); 
        enable_e1(); 
        enable_e2(); 
      break;

#ifdef SDSUPPORT
    case 20: // M20 - list SD card
      SERIAL_PROTOCOLLNPGM(MSG_BEGIN_FILE_LIST);
      card.ls();
      SERIAL_PROTOCOLLNPGM(MSG_END_FILE_LIST);
      break;
    case 21: // M21 - init SD card
      
      card.initsd();
      
      break;
    case 22: //M22 - release SD card
      card.release();

      break;
    case 23: //M23 - Select file
      starpos = (strchr(strchr_pointer + 4,'*'));
      if(starpos!=NULL)
        *(starpos-1)='\0';
      card.openFile(strchr_pointer + 4,true);
      break;
    case 24: //M24 - Start SD print
      card.startFileprint();
      starttime=millis();
      break;
    case 25: //M25 - Pause SD print
      card.pauseSDPrint();
      break;
    case 26: //M26 - Set SD index
      if(card.cardOK && code_seen('S')) {
        card.setIndex(code_value_long());
      }
      break;
    case 27: //M27 - Get SD status
      card.getStatus();
      break;
    case 28: //M28 - Start SD write
      starpos = (strchr(strchr_pointer + 4,'*'));
      if(starpos != NULL){
        char* npos = strchr(cmdbuffer[bufindr], 'N');
        strchr_pointer = strchr(npos,' ') + 1;
        *(starpos-1) = '\0';
      }
      card.openFile(strchr_pointer+4,false);
      break;
    case 29: //M29 - Stop SD write
      //processed in write to file routine above
      //card,saving = false;
      break;
    case 30: //M30 <filename> Delete File 
	if (card.cardOK){
		card.closefile();
		starpos = (strchr(strchr_pointer + 4,'*'));
                if(starpos != NULL){
                char* npos = strchr(cmdbuffer[bufindr], 'N');
                strchr_pointer = strchr(npos,' ') + 1;
                *(starpos-1) = '\0';
         }
	 card.removeFile(strchr_pointer + 4);
	}
	break;
	
#endif //SDSUPPORT

    case 31: //M31 take time since the start of the SD print or an M109 command
      {
      stoptime=millis();
      char time[30];
      unsigned long t=(stoptime-starttime)/1000;
      int sec,min;
      min=t/60;
      sec=t%60;
      sprintf_P(time, PSTR("%i min, %i sec"), min, sec);
      SERIAL_ECHO_START;
      SERIAL_ECHOLN(time);
      lcd_setstatus(time);
      autotempShutdown();
      }
      break;
    case 42: //M42 -Change pin status via gcode
      if (code_seen('S'))
      {
        int pin_status = code_value();
        int pin_number = LED_PIN;
        if (code_seen('P') && pin_status >= 0 && pin_status <= 255)
          pin_number = code_value();
        for(int8_t i = 0; i < (int8_t)sizeof(sensitive_pins); i++)
        {
          if (sensitive_pins[i] == pin_number)
          {
            pin_number = -1;
            break;
          }
        }
        if (pin_number > -1)
        {
          pinMode(pin_number, OUTPUT);
          digitalWrite(pin_number, pin_status);
          analogWrite(pin_number, pin_status);
        }
      }
     break;
    case 104: // M104
      if(setTargetedHotend(104)){
        break;
      }
      if (code_seen('S')) setTargetHotend(code_value(), tmp_extruder);
      setWatch();
      break;
    case 140: // M140 set bed temp
      if (code_seen('S')) setTargetBed(code_value());
      break;
    case 105 : // M105
      if(setTargetedHotend(105)){
        break;
      }
      #if (TEMP_0_PIN > -1)
        SERIAL_PROTOCOLPGM("ok T:");
        SERIAL_PROTOCOL_F(degHotend(tmp_extruder),1); 
        SERIAL_PROTOCOLPGM(" /");
        SERIAL_PROTOCOL_F(degTargetHotend(tmp_extruder),1); 
        #if TEMP_BED_PIN > -1
          SERIAL_PROTOCOLPGM(" B:");  
          SERIAL_PROTOCOL_F(degBed(),1);
          SERIAL_PROTOCOLPGM(" /");
          SERIAL_PROTOCOL_F(degTargetBed(),1);
        #endif //TEMP_BED_PIN
      #else
        SERIAL_ERROR_START;
        SERIAL_ERRORLNPGM(MSG_ERR_NO_THERMISTORS);
      #endif

        SERIAL_PROTOCOLPGM(" @:");
        SERIAL_PROTOCOL(getHeaterPower(tmp_extruder));  

        SERIAL_PROTOCOLPGM(" B@:");
        SERIAL_PROTOCOL(getHeaterPower(-1));  

        SERIAL_PROTOCOLLN("");
      return;
      break;
    case 109: 
    {// M109 - Wait for extruder heater to reach target.
      if(setTargetedHotend(109)){
        break;
      }
      LCD_MESSAGEPGM(MSG_HEATING);   
      #ifdef AUTOTEMP
        autotemp_enabled=false;
      #endif
      if (code_seen('S')) setTargetHotend(code_value(), tmp_extruder);
      #ifdef AUTOTEMP
        if (code_seen('S')) autotemp_min=code_value();
        if (code_seen('B')) autotemp_max=code_value();
        if (code_seen('F')) 
        {
          autotemp_factor=code_value();
          autotemp_enabled=true;
        }
      #endif
      
      setWatch();
      codenum = millis(); 

      /* See if we are heating up or cooling down */
      bool target_direction = isHeatingHotend(tmp_extruder); // true if heating, false if cooling

      #ifdef TEMP_RESIDENCY_TIME
        long residencyStart;
        residencyStart = -1;
        /* continue to loop until we have reached the target temp   
          _and_ until TEMP_RESIDENCY_TIME hasn't passed since we reached it */
        while((residencyStart == -1) ||
              (residencyStart >= 0 && (((unsigned int) (millis() - residencyStart)) < (TEMP_RESIDENCY_TIME * 1000UL))) ) {
      #else
        while ( target_direction ? (isHeatingHotend(tmp_extruder)) : (isCoolingHotend(tmp_extruder)&&(CooldownNoWait==false)) ) {
      #endif //TEMP_RESIDENCY_TIME
          if( (millis() - codenum) > 1000UL )
          { //Print Temp Reading and remaining time every 1 second while heating up/cooling down
            SERIAL_PROTOCOLPGM("T:");
            SERIAL_PROTOCOL_F(degHotend(tmp_extruder),1); 
            SERIAL_PROTOCOLPGM(" E:");
            SERIAL_PROTOCOL((int)tmp_extruder); 
            #ifdef TEMP_RESIDENCY_TIME
              SERIAL_PROTOCOLPGM(" W:");
              if(residencyStart > -1)
              {
                 codenum = ((TEMP_RESIDENCY_TIME * 1000UL) - (millis() - residencyStart)) / 1000UL;
                 SERIAL_PROTOCOLLN( codenum );
              }
              else 
              {
                 SERIAL_PROTOCOLLN( "?" );
              }
            #else
              SERIAL_PROTOCOLLN("");
            #endif
            codenum = millis();
          }
          manage_heater();
          manage_inactivity();
          lcd_update();
        #ifdef TEMP_RESIDENCY_TIME
            /* start/restart the TEMP_RESIDENCY_TIME timer whenever we reach target temp for the first time
              or when current temp falls outside the hysteresis after target temp was reached */
          if ((residencyStart == -1 &&  target_direction && (degHotend(tmp_extruder) >= (degTargetHotend(tmp_extruder)-TEMP_WINDOW))) ||
              (residencyStart == -1 && !target_direction && (degHotend(tmp_extruder) <= (degTargetHotend(tmp_extruder)+TEMP_WINDOW))) ||
              (residencyStart > -1 && labs(degHotend(tmp_extruder) - degTargetHotend(tmp_extruder)) > TEMP_HYSTERESIS) ) 
          {
            residencyStart = millis();
          }
        #endif //TEMP_RESIDENCY_TIME
        }
        LCD_MESSAGEPGM(MSG_HEATING_COMPLETE);
        starttime=millis();
        previous_millis_cmd = millis();
      }
      break;
    case 190: // M190 - Wait for bed heater to reach target.
    #if TEMP_BED_PIN > -1
        LCD_MESSAGEPGM(MSG_BED_HEATING);
        if (code_seen('S')) setTargetBed(code_value());
        codenum = millis(); 
        while(isHeatingBed()) 
        {
          if(( millis() - codenum) > 1000 ) //Print Temp Reading every 1 second while heating up.
          {
            float tt=degHotend(active_extruder);
            SERIAL_PROTOCOLPGM("T:");
            SERIAL_PROTOCOL(tt);
            SERIAL_PROTOCOLPGM(" E:");
            SERIAL_PROTOCOL((int)active_extruder); 
            SERIAL_PROTOCOLPGM(" B:");
            SERIAL_PROTOCOL_F(degBed(),1); 
            SERIAL_PROTOCOLLN(""); 
            codenum = millis(); 
          }
          manage_heater();
          manage_inactivity();
          lcd_update();
        }
        LCD_MESSAGEPGM(MSG_BED_DONE);
        previous_millis_cmd = millis();
    #endif
        break;

    #if FAN_PIN > -1
      case 106: //M106 Fan On
        if (code_seen('S')){
           fanSpeed=constrain(code_value(),0,255);
        }
        else {
          fanSpeed=255;			
        }
        break;
      case 107: //M107 Fan Off
        fanSpeed = 0;
        break;
    #endif //FAN_PIN

    #if (PS_ON_PIN > -1)
      case 80: // M80 - ATX Power On
        SET_OUTPUT(PS_ON_PIN); //GND
        WRITE(PS_ON_PIN, PS_ON_AWAKE);
        break;
      #endif
      
      case 81: // M81 - ATX Power Off
      
      #if defined SUICIDE_PIN && SUICIDE_PIN > -1
        st_synchronize();
        suicide();
      #elif (PS_ON_PIN > -1)
        SET_OUTPUT(PS_ON_PIN); 
        WRITE(PS_ON_PIN, PS_ON_ASLEEP);
      #endif
		break;
        
    case 82:
      axis_relative_modes[3] = false;
      break;
    case 83:
      axis_relative_modes[3] = true;
      break;
    case 18: //compatibility
    case 84: // M84
      if(code_seen('S')){ 
        stepper_inactive_time = code_value() * 1000; 
      }
      else
      { 
        bool all_axis = !((code_seen(axis_codes[0])) || (code_seen(axis_codes[1])) || (code_seen(axis_codes[2]))|| (code_seen(axis_codes[3])));
        if(all_axis)
        {
          st_synchronize();
          disable_e0();
          disable_e1();
          disable_e2();
          finishAndDisableSteppers();
        }
        else
        {
          st_synchronize();
          if(code_seen('X')) disable_x();
          if(code_seen('Y')) disable_y();
          if(code_seen('Z')) disable_z();
          #if ((E0_ENABLE_PIN != X_ENABLE_PIN) && (E1_ENABLE_PIN != Y_ENABLE_PIN)) // Only enable on boards that have seperate ENABLE_PINS
            if(code_seen('E')) {
              disable_e0();
              disable_e1();
              disable_e2();
            }
          #endif 
        }
      }
      break;
    case 85: // M85
      code_seen('S');
      max_inactive_time = code_value() * 1000; 
      break;
    case 92: // M92
      for(int8_t i=0; i < NUM_AXIS; i++) 
      {
        if(code_seen(axis_codes[i])) 
        {
          if(i == 3) { // E
            float value = code_value();
            if(value < 20.0) {
              float factor = axis_steps_per_unit[i] / value; // increase e constants if M92 E14 is given for netfab.
              max_e_jerk *= factor;
              max_feedrate[i] *= factor;
              axis_steps_per_sqr_second[i] *= factor;
            }
            axis_steps_per_unit[i] = value;
          }
          else {
            axis_steps_per_unit[i] = code_value();
          }
        }
      }
      break;
    case 115: // M115
      SERIAL_PROTOCOLPGM(MSG_M115_REPORT);
      break;
    case 117: // M117 display message
      starpos = (strchr(strchr_pointer + 5,'*'));
      if(starpos!=NULL)
        *(starpos-1)='\0';
      lcd_setstatus(strchr_pointer + 5);
      SERIAL_ECHOLN(strchr_pointer + 5);
            
      
      break;
    case 114: // M114
      if (!dCal_X){
        SERIAL_ECHOLN(" *** Home Pending ***");
      }  
      SERIAL_PROTOCOLPGM("X:");
      SERIAL_PROTOCOL(current_position[X_AXIS]);
      SERIAL_PROTOCOLPGM("Y:");
      SERIAL_PROTOCOL(current_position[Y_AXIS]);
      SERIAL_PROTOCOLPGM("Z:");
      SERIAL_PROTOCOL(current_position[Z_AXIS]);
      SERIAL_PROTOCOLPGM("E:");      
      SERIAL_PROTOCOL(current_position[E_AXIS]);

      SERIAL_PROTOCOLLN("");
      SERIAL_PROTOCOLPGM("SCARA Theta:");
      SERIAL_PROTOCOL(delta[X_AXIS]+add_homeing[0]);
      SERIAL_PROTOCOLPGM("   Psi+Theta:");
      SERIAL_PROTOCOL(delta[Y_AXIS]+add_homeing[1]);
      SERIAL_PROTOCOLLN("");
      
      SERIAL_PROTOCOLPGM("SCARA Cal - Theta:");
      SERIAL_PROTOCOL(delta[X_AXIS]);
      SERIAL_PROTOCOLPGM("   Psi+Theta (90):");
      SERIAL_PROTOCOL(delta[Y_AXIS]-delta[X_AXIS]-90);
      SERIAL_PROTOCOLLN("");
      
      SERIAL_PROTOCOLPGM("SCARA step Cal - Theta:");
      SERIAL_PROTOCOL(delta[X_AXIS]/90*axis_steps_per_unit[X_AXIS]);
      SERIAL_PROTOCOLPGM("   Psi+Theta:");
      SERIAL_PROTOCOL((delta[Y_AXIS]-delta[X_AXIS])/90*axis_steps_per_unit[Y_AXIS]);
      SERIAL_PROTOCOLLN("");
      SERIAL_PROTOCOLLN("");
      
      break;
    case 120: // M120
      enable_endstops(false) ;
      break;
    case 121: // M121
      enable_endstops(true) ;
      break;
    case 119: // M119
    SERIAL_PROTOCOLLN(MSG_M119_REPORT);
      //SERIAL_ECHOLN();    //DEBUG
      //WRITE(X_MIN_PIN,1);
      #if (X_MIN_PIN > -1)
        SERIAL_PROTOCOLPGM(MSG_X_MIN);
        SERIAL_PROTOCOLLN(((READ(X_MIN_PIN)^X_ENDSTOPS_INVERTING)?MSG_ENDSTOP_HIT:MSG_ENDSTOP_OPEN));
      #endif
      #if (X_MAX_PIN > -1)
        SERIAL_PROTOCOLPGM(MSG_X_MAX);
        SERIAL_PROTOCOLLN(((READ(X_MAX_PIN)^X_ENDSTOPS_INVERTING)?MSG_ENDSTOP_HIT:MSG_ENDSTOP_OPEN));
      #endif
      #if (Y_MIN_PIN > -1)
        SERIAL_PROTOCOLPGM(MSG_Y_MIN);
        SERIAL_PROTOCOLLN(((READ(Y_MIN_PIN)^Y_ENDSTOPS_INVERTING)?MSG_ENDSTOP_HIT:MSG_ENDSTOP_OPEN));
      #endif
      #if (Y_MAX_PIN > -1)
        SERIAL_PROTOCOLPGM(MSG_Y_MAX);
        SERIAL_PROTOCOLLN(((READ(Y_MAX_PIN)^Y_ENDSTOPS_INVERTING)?MSG_ENDSTOP_HIT:MSG_ENDSTOP_OPEN));
      #endif
      #if (Z_MIN_PIN > -1)
        SERIAL_PROTOCOLPGM(MSG_Z_MIN);
        SERIAL_PROTOCOLLN(((READ(Z_MIN_PIN)^Z_ENDSTOPS_INVERTING)?MSG_ENDSTOP_HIT:MSG_ENDSTOP_OPEN));
      #endif
      #if (Z_MAX_PIN > -1)
        SERIAL_PROTOCOLPGM(MSG_Z_MAX);
        SERIAL_PROTOCOLLN(((READ(Z_MAX_PIN)^Z_ENDSTOPS_INVERTING)?MSG_ENDSTOP_HIT:MSG_ENDSTOP_OPEN));
      #endif
      break;
      //TODO: update for all axis, use for loop
    case 201: // M201
      for(int8_t i=0; i < NUM_AXIS; i++) 
      {
        if(code_seen(axis_codes[i]))
        {
          max_acceleration_units_per_sq_second[i] = code_value();
          axis_steps_per_sqr_second[i] = code_value() * axis_steps_per_unit[i];
        }
      }
      break;
    #if 0 // Not used for Sprinter/grbl gen6
    case 202: // M202
      for(int8_t i=0; i < NUM_AXIS; i++) {
        if(code_seen(axis_codes[i])) axis_travel_steps_per_sqr_second[i] = code_value() * axis_steps_per_unit[i];
      }
      break;
    #endif
    case 203: // M203 max feedrate mm/sec
      for(int8_t i=0; i < NUM_AXIS; i++) {
        if(code_seen(axis_codes[i])) max_feedrate[i] = code_value();
      }
      break;
    case 204: // M204 acclereration S normal moves T filmanent only moves
      {
        if(code_seen('S')) acceleration = code_value() ;
        if(code_seen('T')) retract_acceleration = code_value() ;
      }
      break;
    case 205: //M205 advanced settings:  minimum travel speed S=while printing T=travel only,  B=minimum segment time X= maximum xy jerk, Z=maximum Z jerk
    {
      if(code_seen('S')) minimumfeedrate = code_value();
      if(code_seen('T')) mintravelfeedrate = code_value();
      if(code_seen('B')) minsegmenttime = code_value() ;
      if(code_seen('X')) max_xy_jerk = code_value() ;
      if(code_seen('Z')) max_z_jerk = code_value() ;
      if(code_seen('E')) max_e_jerk = code_value() ;
    }
    break;
    case 206: // M206 additional homeing offset
      for(int8_t i=0; i < 3; i++) 
      {
        if(code_seen(axis_codes[i])) add_homeing[i] = code_value();
      }
      
      if(code_seen('T'))       // Theta
      {
        add_homeing[0] = code_value() ;
      }
      
      if(code_seen('P'))       // Psi
      {
        add_homeing[1] = code_value() ;
      }
      break;
    #ifdef FWRETRACT
    case 207: //M207 - set retract length S[positive mm] F[feedrate mm/sec] Z[additional zlift/hop]
    {
      if(code_seen('S')) 
      {
        retract_length = code_value() ;
      }
      if(code_seen('F')) 
      {
        retract_feedrate = code_value() ;
      }
      if(code_seen('Z')) 
      {
        retract_zlift = code_value() ;
      }
    }break;
    case 208: // M208 - set retract recover length S[positive mm surplus to the M207 S*] F[feedrate mm/sec]
    {
      if(code_seen('S')) 
      {
        retract_recover_length = code_value() ;
      }
      if(code_seen('F')) 
      {
        retract_recover_feedrate = code_value() ;
      }
    }break;
    
    case 209: // M209 - S<1=true/0=false> enable automatic retract detect if the slicer did not support G10/11: every normal extrude-only move will be classified as retract depending on the direction.
    {
      if(code_seen('S')) 
      {
        int t= code_value() ;
        switch(t)
        {
          case 0: autoretract_enabled=false;retracted=false;break;
          case 1: autoretract_enabled=true;retracted=false;break;
          default: 
            SERIAL_ECHO_START;
            SERIAL_ECHOPGM(MSG_UNKNOWN_COMMAND);
            SERIAL_ECHO(cmdbuffer[bufindr]);
            SERIAL_ECHOLNPGM("\"");
        }
      }
      
    }break;
    #endif
    case 220: // M220 S<factor in percent>- set speed factor override percentage
    {
      if(code_seen('S')) 
      {
        feedmultiply = code_value() ;
      }
    }
    break;
    case 221: // M221 S<factor in percent>- set extrude factor override percentage
    {
      if(code_seen('S')) 
      {
        extrudemultiply = code_value() ;
      }
    }
    break;

    #ifdef PIDTEMP
    case 301: // M301
      {
        if(code_seen('P')) Kp = code_value();
        if(code_seen('I')) Ki = code_value()*PID_dT;
        if(code_seen('D')) Kd = code_value()/PID_dT;
        #ifdef PID_ADD_EXTRUSION_RATE
        if(code_seen('C')) Kc = code_value();
        #endif
        updatePID();
        SERIAL_PROTOCOL(MSG_OK);
		SERIAL_PROTOCOL(" p:");
        SERIAL_PROTOCOL(Kp);
        SERIAL_PROTOCOL(" i:");
        SERIAL_PROTOCOL(Ki/PID_dT);
        SERIAL_PROTOCOL(" d:");
        SERIAL_PROTOCOL(Kd*PID_dT);
        #ifdef PID_ADD_EXTRUSION_RATE
        SERIAL_PROTOCOL(" c:");
        SERIAL_PROTOCOL(Kc*PID_dT);
        #endif
        SERIAL_PROTOCOLLN("");
      }
      break;
    #endif //PIDTEMP
    #ifdef PIDTEMPBED
    case 304: // M304
      {
        if(code_seen('P')) bedKp = code_value();
        if(code_seen('I')) bedKi = code_value()*PID_dT;
        if(code_seen('D')) bedKd = code_value()/PID_dT;
        updatePID();
        SERIAL_PROTOCOL(MSG_OK);
		SERIAL_PROTOCOL(" p:");
        SERIAL_PROTOCOL(bedKp);
        SERIAL_PROTOCOL(" i:");
        SERIAL_PROTOCOL(bedKi/PID_dT);
        SERIAL_PROTOCOL(" d:");
        SERIAL_PROTOCOL(bedKd*PID_dT);
        SERIAL_PROTOCOLLN("");
      }
      break;
    #endif //PIDTEMP
    case 240: // M240  Triggers a camera by emulating a Canon RC-1 : http://www.doc-diy.net/photo/rc-1_hacked/
     {
      #ifdef PHOTOGRAPH_PIN
        #if (PHOTOGRAPH_PIN > -1)
        const uint8_t NUM_PULSES=16;
        const float PULSE_LENGTH=0.01524;
        for(int i=0; i < NUM_PULSES; i++) {
          WRITE(PHOTOGRAPH_PIN, HIGH);
          _delay_ms(PULSE_LENGTH);
          WRITE(PHOTOGRAPH_PIN, LOW);
          _delay_ms(PULSE_LENGTH);
        }
        delay(7.33);
        for(int i=0; i < NUM_PULSES; i++) {
          WRITE(PHOTOGRAPH_PIN, HIGH);
          _delay_ms(PULSE_LENGTH);
          WRITE(PHOTOGRAPH_PIN, LOW);
          _delay_ms(PULSE_LENGTH);
        }
        #endif
      #endif
     }
    break;
      
    case 302: // allow cold extrudes
    {
      allow_cold_extrudes(true);
    }
    break;
    case 303: // M303 PID autotune
    {
      float temp = 150.0;
      int e=0;
      int c=5;
      if (code_seen('E')) e=code_value();
			if (e<0)
				temp=70;
      if (code_seen('S')) temp=code_value();
      if (code_seen('C')) c=code_value();
      PID_autotune(temp, e, c);
    }
    break;
    case 400: // M400 finish all moves
    {
      st_synchronize();
    }
    break;
    case 500: // M500 Store settings in EEPROM
    {
        SoftEndsEnabled = true;              // Ignore soft endstops during calibration
      SERIAL_ECHOLN(" Soft endstops enabled ");
      
        Config_StoreSettings();
    }
    break;
    case 501: // M501 Read settings from EEPROM
    {
        Config_RetrieveSettings();
    }
    break;
    case 502: // M502 Revert to default settings
    {
        Config_ResetDefault();
    }
    break;
    case 503: // M503 print settings currently in memory
    {
        Config_PrintSettings();
    }
    break;
    #ifdef ABORT_ON_ENDSTOP_HIT_FEATURE_ENABLED
    case 540:
    {
        if(code_seen('S')) abort_on_endstop_hit = code_value() > 0;
    }
    break;
    #endif
    #ifdef FILAMENTCHANGEENABLE
    case 600: //Pause for filament change X[pos] Y[pos] Z[relative lift] E[initial retract] L[later retract distance for removal]
    {
        float target[4];
        float lastpos[4];
        target[X_AXIS]=current_position[X_AXIS];
        target[Y_AXIS]=current_position[Y_AXIS];
        target[Z_AXIS]=current_position[Z_AXIS];
        target[E_AXIS]=current_position[E_AXIS];
        lastpos[X_AXIS]=current_position[X_AXIS];
        lastpos[Y_AXIS]=current_position[Y_AXIS];
        lastpos[Z_AXIS]=current_position[Z_AXIS];
        lastpos[E_AXIS]=current_position[E_AXIS];
        //retract by E
        if(code_seen('E')) 
        {
          target[E_AXIS]+= code_value();
        }
        else
        {
          #ifdef FILAMENTCHANGE_FIRSTRETRACT
            target[E_AXIS]+= FILAMENTCHANGE_FIRSTRETRACT ;
          #endif
        }
        plan_buffer_line(target[X_AXIS], target[Y_AXIS], target[Z_AXIS], target[E_AXIS], feedrate/60, active_extruder);
        
        //lift Z
        if(code_seen('Z')) 
        {
          target[Z_AXIS]+= code_value();
        }
        else
        {
          #ifdef FILAMENTCHANGE_ZADD
            target[Z_AXIS]+= FILAMENTCHANGE_ZADD ;
          #endif
        }
        plan_buffer_line(target[X_AXIS], target[Y_AXIS], target[Z_AXIS], target[E_AXIS], feedrate/60, active_extruder);
        
        //move xy
        if(code_seen('X')) 
        {
          target[X_AXIS]+= code_value();
        }
        else
        {
          #ifdef FILAMENTCHANGE_XPOS
            target[X_AXIS]= FILAMENTCHANGE_XPOS ;
          #endif
        }
        if(code_seen('Y')) 
        {
          target[Y_AXIS]= code_value();
        }
        else
        {
          #ifdef FILAMENTCHANGE_YPOS
            target[Y_AXIS]= FILAMENTCHANGE_YPOS ;
          #endif
        }
        
        plan_buffer_line(target[X_AXIS], target[Y_AXIS], target[Z_AXIS], target[E_AXIS], feedrate/60, active_extruder);
        
        if(code_seen('L'))
        {
          target[E_AXIS]+= code_value();
        }
        else
        {
          #ifdef FILAMENTCHANGE_FINALRETRACT
            target[E_AXIS]+= FILAMENTCHANGE_FINALRETRACT ;
          #endif
        }
        
        plan_buffer_line(target[X_AXIS], target[Y_AXIS], target[Z_AXIS], target[E_AXIS], feedrate/60, active_extruder);
        
        //finish moves
        st_synchronize();
        //disable extruder steppers so filament can be removed
        disable_e0();
        disable_e1();
        disable_e2();
        delay(100);
        LCD_ALERTMESSAGEPGM(MSG_FILAMENTCHANGE);
        uint8_t cnt=0;
        while(!LCD_CLICKED){
          cnt++;
          manage_heater();
          manage_inactivity();
          lcd_update();
          
          #if BEEPER > -1
          if(cnt==0)
          {
            SET_OUTPUT(BEEPER);
            
            WRITE(BEEPER,HIGH);
            delay(3);
            WRITE(BEEPER,LOW);
            delay(3);
          }
          #endif
        }
        
        //return to normal
        if(code_seen('L')) 
        {
          target[E_AXIS]+= -code_value();
        }
        else
        {
          #ifdef FILAMENTCHANGE_FINALRETRACT
            target[E_AXIS]+=(-1)*FILAMENTCHANGE_FINALRETRACT ;
          #endif
        }
        current_position[E_AXIS]=target[E_AXIS]; //the long retract of L is compensated by manual filament feeding
        plan_set_e_position(current_position[E_AXIS]);
        plan_buffer_line(target[X_AXIS], target[Y_AXIS], target[Z_AXIS], target[E_AXIS], feedrate/60, active_extruder); //should do nothing
        plan_buffer_line(lastpos[X_AXIS], lastpos[Y_AXIS], target[Z_AXIS], target[E_AXIS], feedrate/60, active_extruder); //move xy back
        plan_buffer_line(lastpos[X_AXIS], lastpos[Y_AXIS], lastpos[Z_AXIS], target[E_AXIS], feedrate/60, active_extruder); //move z back
        plan_buffer_line(lastpos[X_AXIS], lastpos[Y_AXIS], lastpos[Z_AXIS], lastpos[E_AXIS], feedrate/60, active_extruder); //final untretract
    }
    break;
    #endif //FILAMENTCHANGEENABLE    
    case 907: // M907 Set digital trimpot motor current using axis codes.
    {
      #if DIGIPOTSS_PIN > -1
        for(int i=0;i<=NUM_AXIS;i++) if(code_seen(axis_codes[i])) digipot_current(i,code_value());
        if(code_seen('B')) digipot_current(4,code_value());
        if(code_seen('S')) for(int i=0;i<=4;i++) digipot_current(i,code_value());
      #endif
    }
    case 908: // M908 Control digital trimpot directly.
    {
      #if DIGIPOTSS_PIN > -1
        uint8_t channel,current;
        if(code_seen('P')) channel=code_value();
        if(code_seen('S')) current=code_value();
        digitalPotWrite(channel, current);
      #endif
    }
    break;
    case 350: // M350 Set microstepping mode. Warning: Steps per unit remains unchanged. S code sets stepping mode for all drivers.
    {
      #if X_MS1_PIN > -1
        if(code_seen('S')) for(int i=0;i<=4;i++) microstep_mode(i,code_value()); 
        for(int i=0;i<=NUM_AXIS;i++) if(code_seen(axis_codes[i])) microstep_mode(i,(uint8_t)code_value());
        if(code_seen('B')) microstep_mode(4,code_value());
        microstep_readings();
      #endif
    }
    break;
    case 351: // M351 Toggle MS1 MS2 pins directly, S# determines MS1 or MS2, X# sets the pin high/low.
    {
      #if X_MS1_PIN > -1
      if(code_seen('S')) switch((int)code_value())
      {
        case 1:
          for(int i=0;i<=NUM_AXIS;i++) if(code_seen(axis_codes[i])) microstep_ms(i,code_value(),-1);
          if(code_seen('B')) microstep_ms(4,code_value(),-1);
          break;
        case 2:
          for(int i=0;i<=NUM_AXIS;i++) if(code_seen(axis_codes[i])) microstep_ms(i,-1,code_value());
          if(code_seen('B')) microstep_ms(4,-1,code_value());
          break;
      }
      microstep_readings();
      #endif
    }
    break;
    case 360:  // SCARA Theta pos1
      SERIAL_ECHOLN(" Cal: Theta 0 ");
      SoftEndsEnabled = false;              // Ignore soft endstops during calibration
      SERIAL_ECHOLN(" Soft endstops disabled ");
      if(Stopped == false && dCal_X) {

        delta[0] = 0;			// Move Theta to 0 degrees (arm pointing west if you are looking at the printer with the bed between you and the z-axis motor)
        delta[1] = 120;			// Move Psi to 120 degrees 
        calculate_forward(delta);
        destination[0] = delta[0];
        destination[1] = delta[1];
        
        prepare_move();
        return;
      }
    break;
    case 361:  // SCARA Theta pos2
      SERIAL_ECHOLN(" Cal: Theta 90 ");
      SoftEndsEnabled = false;              // Ignore soft endstops during calibration
      SERIAL_ECHOLN(" Soft endstops disabled ");
      if(Stopped == false && dCal_X) {
        //get_coordinates(); // For X Y Z E F
        delta[0] = 90;
        delta[1] = 130;
        calculate_forward(delta);
        destination[0] = delta[0];
        destination[1] = delta[1];
        
        prepare_move();
        //ClearToSend();
        return;
      }
    break;
    case 362:  // SCARA Psi pos1
      SERIAL_ECHOLN(" Cal: Psi 0 ");
      SoftEndsEnabled = false;              // Ignore soft endstops during calibration
      SERIAL_ECHOLN(" Soft endstops disabled ");
      if(Stopped == false && dCal_X) {
        //get_coordinates(); // For X Y Z E F
        delta[0] = 60;
        delta[1] = 180;
        calculate_forward(delta);
        destination[0] = delta[0];
        destination[1] = delta[1];
        
        prepare_move();
        //ClearToSend();
        return;
      }
    break;
    case 363:  // SCARA Psi pos2
      SERIAL_ECHOLN(" Cal: Psi 90 ");
      SoftEndsEnabled = false;              // Ignore soft endstops during calibration
      SERIAL_ECHOLN(" Soft endstops disabled ");
      if(Stopped == false && dCal_X) {
        //get_coordinates(); // For X Y Z E F
        delta[0] = 50;
        delta[1] = 90;
        calculate_forward(delta);
        destination[0] = delta[0];
        destination[1] = delta[1];
        
        prepare_move();
        //ClearToSend();
        return;
      }
    break;
    case 364:  // SCARA Psi pos3 (90 deg to Theta)
      SERIAL_ECHOLN(" Cal: Theta-Psi 90 ");
      SoftEndsEnabled = false;              // Ignore soft endstops during calibration
      SERIAL_ECHOLN(" Soft endstops disabled ");
      if(Stopped == false && dCal_X) {
        //get_coordinates(); // For X Y Z E F
        delta[0] = 45;
        delta[1] = 135;
        calculate_forward(delta);
        destination[0] = delta[0];
        destination[1] = delta[1]; 
        
        prepare_move();
        //ClearToSend();
        return;
      }
    break;
    case 365: // M364  Set SCARA scaling for X Y Z
      for(int8_t i=0; i < 3; i++) 
      {
        if(code_seen(axis_codes[i])) 
        {
          
            axis_scaling[i] = code_value();
          
        }
      }
      break;
    case 366:  // SCARA Move to Theta and Psi position
      SERIAL_ECHOLN(" Cal: Move to location  ");
      SoftEndsEnabled = false;              // Ignore soft endstops during calibration
      SERIAL_ECHOLN(" Soft endstops disabled ");
      if(Stopped == false && dCal_X) {
        get_coordinates(false); // For X Y Z E F
        delta[0] = destination[0];
        delta[1] = destination[1];
        calculate_forward(delta);
        destination[0] = delta[0];
        destination[1] = delta[1];
        
        prepare_move();
        return;
      }
    break;
    case 370:  // M370  Initialise Bed Level to Zero and move the head to home position
      
      HomeAllAxis();
      
      Y_gridcal = true;
      
      if(code_seen('X')) GCal_X = code_value()-1;  // Manually specify number of Cal points per side. Uneven number recommended.
      if(code_seen('Y')) GCal_Y = code_value()-1;

      GPos_X = 0;
      GPos_Y = 0;
      dCal_X = X_MAX_POS / GCal_X;                   // Initialise dCal_X & dCal_Y
      dCal_X_Inv = 1.0 / dCal_X;
      dCal_Y = Y_MAX_POS / GCal_Y;
      dCal_Y_Inv = 1.0 / dCal_Y;
      
      // If the C parameter is specified, then  reset the bed level
      if(code_seen('C'))
      {
        for (counterx = 0; counterx < X_ARMLOOKUP_LENGTH; counterx++)
        {
          for (countery = 0; countery < Y_ARMLOOKUP_LENGTH; countery++)
          {
            Arm_lookup[counterx][countery] = 0;
          }  
        }

        SERIAL_ECHO_START;
        SERIAL_ECHOLN(" Y-level grid cleared  ");
      }
      else
      {
        SERIAL_ECHO_START;
        SERIAL_ECHOLN(" Using Y-level grid from previous calibration. Use M370 C to clear it.  ");
      }
      
      ZCalMoveIntoToPosition();
      
      break;
      
    case 372:  // M372   Program current zero position into levelling grid.
    
      SERIAL_ECHO_START;
        
      if(Y_gridcal)        // only program when gridcal set
      {
        SERIAL_ECHOLN("Storing lookup for:");
        SERIAL_ECHO(" X:");
        SERIAL_ECHO(current_position[X_AXIS]);
        SERIAL_ECHO(" Y:");
        SERIAL_ECHO(current_position[Y_AXIS]);
        SERIAL_ECHO(" Z:");
        SERIAL_ECHOLN(current_position[Z_AXIS]);
      
        Arm_lookup[(int)(current_position[X_AXIS]/X_MAX_POS * GCal_X)][(int)(current_position[Y_AXIS]/Y_MAX_POS * GCal_Y)] = current_position[Z_AXIS];
        SERIAL_ECHO(" - ");
        SERIAL_ECHOLN("OK");
      }  
        else
        {
          SERIAL_ECHOLN("No GridCal");
        }
      
    //break;  // No break: Move to next position automatically
      
    case 371:  // M371   Move to next position on the grid ready for allignment  
      
      if (Y_gridcal)
      {
        GPos_X++;
        
        if ((GPos_X * X_MAX_POS / GCal_X) > X_MAX_POS){
          GPos_X = 0;
          GPos_Y++;
          if ((GPos_Y * Y_MAX_POS / GCal_Y) > Y_MAX_POS){
            GPos_Y = 0;
            SERIAL_ECHO(" - ");
            SERIAL_ECHOLN("Last calibration point...");
          }
        }

        ZCalMoveIntoToPosition();
      }
          
    break;      
      
  
    case 373: // end Grid calibration
      
      Y_gridcal = false;
  
    break;  
 
    
    case 375:  // debug: print grid to serial  
        for (countery = 0; countery < Y_ARMLOOKUP_LENGTH; countery++)
        {
           for (countery = 0; countery < Y_ARMLOOKUP_LENGTH; countery++)
           for (counterx = 0; counterx < X_ARMLOOKUP_LENGTH; counterx++)
           {
             SERIAL_ECHOPAIR(" " , Arm_lookup[counterx][countery] ); 
              
           }
           //EEPROM_WRITE_VAR(i,Arm_lookup[counterx]);        // Z-arm corrcetion save
    
        }
  
    break;  
    
    case 376:  // debug: print grid to file  

        for (countery = 0; countery < Y_ARMLOOKUP_LENGTH; countery++)
        {
           for (countery = 0; countery < Y_ARMLOOKUP_LENGTH; countery++)
           for (counterx = 0; counterx < X_ARMLOOKUP_LENGTH; counterx++)
           {
             SERIAL_ECHOPAIR(" " , Arm_lookup[counterx][countery] ); 
              
           }
           //EEPROM_WRITE_VAR(i,Arm_lookup[counterx]);        // Z-arm corrcetion save
    
        }
  
    break;  
      
    case 999: // M999: Restart after being stopped
      Stopped = false;
      lcd_reset_alert_level();
      gcode_LastN = Stopped_gcode_LastN;
      FlushSerialRequestResend();
    break;
    }
  }

  else if(code_seen('T')) 
  {
    tmp_extruder = code_value();
    if(tmp_extruder >= EXTRUDERS) {
      SERIAL_ECHO_START;
      SERIAL_ECHO("T");
      SERIAL_ECHO(tmp_extruder);
      SERIAL_ECHOLN(MSG_INVALID_EXTRUDER);
    }
    else {
      active_extruder = tmp_extruder;
      SERIAL_ECHO_START;
      SERIAL_ECHO(MSG_ACTIVE_EXTRUDER);
      SERIAL_PROTOCOLLN((int)active_extruder);
    }
  }

  else
  {
    SERIAL_ECHO_START;
    SERIAL_ECHOPGM(MSG_UNKNOWN_COMMAND);
    SERIAL_ECHO(cmdbuffer[bufindr]);
    SERIAL_ECHOLNPGM("\"");
  }

  ClearToSend();
}

void FlushSerialRequestResend()
{
  //char cmdbuffer[bufindr][100]="Resend:";
  MYSERIAL.flush();
  SERIAL_PROTOCOLPGM(MSG_RESEND);
  SERIAL_PROTOCOLLN(gcode_LastN + 1);
  ClearToSend();
}

void ClearToSend()
{
  previous_millis_cmd = millis();
  #ifdef SDSUPPORT
  if(fromsd[bufindr])
    return;
  #endif //SDSUPPORT
  SERIAL_PROTOCOLLNPGM(MSG_OK); 
}

void get_coordinates(bool apply_scaling)
{
  bool seen[4]={false,false,false,false};
  for(int8_t i=0; i < NUM_AXIS; i++) {
    if(code_seen(axis_codes[i])) 
    {
      float scale = (apply_scaling) ? axis_scaling[i] : 1.0;
      destination[i] = (float)code_value()*scale + (axis_relative_modes[i] || relative_mode)*current_position[i];
      seen[i]=true;
    }
    else destination[i] = current_position[i]; //Are these else lines really needed?
  }
  if(code_seen('F')) {
    next_feedrate = code_value();
    if(next_feedrate > 0.0) feedrate = next_feedrate;
  }
  #ifdef FWRETRACT
  if(autoretract_enabled)
  if( !(seen[X_AXIS] || seen[Y_AXIS] || seen[Z_AXIS]) && seen[E_AXIS])
  {
    float echange=destination[E_AXIS]-current_position[E_AXIS];
    if(echange<-MIN_RETRACT) //retract
    {
      if(!retracted) 
      {
      
      destination[Z_AXIS]+=retract_zlift; //not sure why chaninging current_position negatively does not work.
      //if slicer retracted by echange=-1mm and you want to retract 3mm, corrrectede=-2mm additionally
      float correctede=-echange-retract_length;
      //to generate the additional steps, not the destination is changed, but inversely the current position
      current_position[E_AXIS]+=-correctede; 
      feedrate=retract_feedrate;
      retracted=true;
      }
      
    }
    else 
      if(echange>MIN_RETRACT) //retract_recover
    {
      if(retracted) 
      {
      //current_position[Z_AXIS]+=-retract_zlift;
      //if slicer retracted_recovered by echange=+1mm and you want to retract_recover 3mm, corrrectede=2mm additionally
      float correctede=-echange+1*retract_length+retract_recover_length; //total unretract=retract_length+retract_recover_length[surplus]
      current_position[E_AXIS]+=correctede; //to generate the additional steps, not the destination is changed, but inversely the current position
      feedrate=retract_recover_feedrate;
      retracted=false;
      }
    }
    
  }
  #endif //FWRETRACT
}

void get_arc_coordinates()
{
#ifdef SF_ARC_FIX
   bool relative_mode_backup = relative_mode;
   relative_mode = true;
#endif
   get_coordinates(true);
#ifdef SF_ARC_FIX
   relative_mode=relative_mode_backup;
#endif

   if(code_seen('I')) {
     offset[0] = code_value();
   } 
   else {
     offset[0] = 0.0;
   }
   if(code_seen('J')) {
     offset[1] = code_value();
   }
   else {
     offset[1] = 0.0;
   }
}

void clamp_to_software_endstops(float target[3])
{
  if (min_software_endstops) {
    if (target[X_AXIS] < min_pos[X_AXIS]) target[X_AXIS] = min_pos[X_AXIS];
    if (target[Y_AXIS] < min_pos[Y_AXIS]) target[Y_AXIS] = min_pos[Y_AXIS];
    if (target[Z_AXIS] < min_pos[Z_AXIS]) target[Z_AXIS] = min_pos[Z_AXIS];
  }

  if (max_software_endstops) {
    if (target[X_AXIS] > max_pos[X_AXIS]) target[X_AXIS] = max_pos[X_AXIS];
    if (target[Y_AXIS] > max_pos[Y_AXIS]) target[Y_AXIS] = max_pos[Y_AXIS];
    if (target[Z_AXIS] > max_pos[Z_AXIS]) target[Z_AXIS] = max_pos[Z_AXIS];
  }
}


/*
int calculate_YGrid()
{
  int counterx, countery,
      counterx2, countery2,
      counterx3, countery3,
      delta_n, countvalues = 0, countedge = 0;
  float  deltacalc;
  
  //Grid_temp = Arm_lookup; 
  
  //for (counterx = 0; counterx < X_ARMLOOKUP_LENGTH; counterx++)
  //      {
  //         for (countery = 0; countery < Y_ARMLOOKUP_LENGTH; countery++)
  //        {
   //          Grid_temp[counterx][countery] = Arm_lookup[counterx][countery];
   //        }  
   //     }
  //SERIAL_ECHOLN(" Copied arrays");
  
  if (Y_gridcal)    // only calculate when gridcal set
      {
        for (counterx = 0; counterx < X_ARMLOOKUP_LENGTH; counterx++)
        {
           for (countery = 0; countery < Y_ARMLOOKUP_LENGTH; countery++)
           {   
               if(Arm_lookup[counterx][countery] != 0)
               {
                  counterx2 = counterx+1;
                  while(Arm_lookup[counterx2][countery] == 0 && counterx2 < X_ARMLOOKUP_LENGTH)  // Look for next related value --> X
                  {
                     counterx2++; 
                  }
                  
                  if(counterx2 < X_ARMLOOKUP_LENGTH)
                  {
                    delta_n = counterx2 - counterx;
                    deltacalc = (Arm_lookup[counterx2][countery] - Arm_lookup[counterx][countery])/delta_n;
                    for (counterx3 = counterx+1; counterx3 < counterx2; counterx3++)
                    {
                       Arm_lookup[counterx3][countery] =  Arm_lookup[counterx][countery] + (deltacalc * (counterx3-counterx));  // populate grid up to next calibrated value
                       countvalues++;
                    }
                  }
                  
                  countery2 = countery+1;
                  while(Arm_lookup[counterx][countery2] == 0 && countery2 < Y_ARMLOOKUP_LENGTH)  // Look for next related value --> Y
                  {
                     countery2++; 
                  }
                  
                  if(countery2 < Y_ARMLOOKUP_LENGTH)
                  {
                    delta_n = countery2 - countery;
                    deltacalc = (Arm_lookup[counterx][countery2] - Arm_lookup[counterx][countery])/delta_n;
                    for (countery3 = countery+1; countery3 < countery2; countery3++)
                    {
                       Arm_lookup[counterx][countery3] =  Arm_lookup[counterx][countery] + (deltacalc * (countery3-countery));  // populate grid up to next calibrated value
                       countvalues++;
                    }
                  }
                  
                  // Values for X and Y up to next calibrated value populated in Grid_temp.
                  // Go to next populated value.
                  SERIAL_ECHO(" *");
                  
               }
           }  
        }      // end of linear fill = All intercalibrated values should be populated.
        
        SERIAL_ECHOLN(" Linear fill completed");
  
#if false       
        for (counterx = X_ARMLOOKUP_LENGTH/2; counterx >= 0; counterx--)
        {
          for (countery = Y_ARMLOOKUP_LENGTH/2; countery >= 0; countery--)
          {
            if(Arm_lookup[counterx][countery]==0 && Arm_lookup[counterx+1][countery] != 0)      // leading X
            {
                Arm_lookup[counterx][countery] = Arm_lookup[counterx+1][countery] + Arm_lookup[counterx+1][countery] - Arm_lookup[counterx+2][countery];
            }
            
            if(Arm_lookup[counterx][countery]==0 && Arm_lookup[counterx][countery+1] != 0)      // leading Y
            {
                Arm_lookup[counterx][countery] = Arm_lookup[counterx][countery+1] + Arm_lookup[counterx][countery+1] - Arm_lookup[counterx][countery+2];
            }
          }  
       }
        
        
        for (counterx = X_ARMLOOKUP_LENGTH/2; counterx < X_ARMLOOKUP_LENGTH; counterx++)
        {
          for (countery = Y_ARMLOOKUP_LENGTH/2; countery < Y_ARMLOOKUP_LENGTH; countery++)
          {
            if(Arm_lookup[counterx][countery]==0 && Arm_lookup[counterx-1][countery] != 0)      // trailing X
            {
                Arm_lookup[counterx][countery] = Arm_lookup[counterx-1][countery] + Arm_lookup[counterx-1][countery] - Arm_lookup[counterx-2][countery];
            }
            
            if(Arm_lookup[counterx][countery]==0 && Arm_lookup[counterx][countery-1] != 0)      // trailing Y
            {
                Arm_lookup[counterx][countery] = Arm_lookup[counterx][countery-1] + Arm_lookup[counterx][countery-1] - Arm_lookup[counterx][countery-2];
            }
          }  
       }
#else

      for (counterx = 0; counterx < X_ARMLOOKUP_LENGTH; counterx++)
        {
           for (countery = 0; countery < Y_ARMLOOKUP_LENGTH; countery++)
          {
             if(Arm_lookup[counterx][countery]==0)
             {
                if (counterx > X_MAX_POS) Arm_lookup[counterx][countery] = Arm_lookup[counterx-1][countery];
                else if (countery > Y_MAX_POS) Arm_lookup[counterx][countery] = Arm_lookup[counterx][countery-1];
                else Arm_lookup[counterx][countery] = 10;          // ****************  Temporary cheat !!!!!!!!!!!!!!!!!!!!!! 
             }
           }  
        }
 
#endif
       
       
        
        
        SERIAL_ECHOLN(" Calibration done");
  
        
      }
      else
      {
        SERIAL_ECHOLN(" Not in Calibration mode");
  
      
      }
        
   SERIAL_ECHOLN(" OK");
  
   return countvalues;  // return number of completed grid values
  
}
*/

#define RADIANS(x) ((x) * (1.0/SCARA_RAD2DEG))
#define sqr(x) ((x)*(x))

void calculate_forward(float f_delta[3])
{
  // Perform forward kinematics, and place results in delta[0]
  
  float rho, D, C1, P1, B2, C2, s, p;
  
  s = sqrt(sqr(LengthTheta) + sqr(LengthPsi) - 2 * LengthTheta * LengthPsi * cos(RADIANS(f_delta[Y_AXIS] - f_delta[X_AXIS])));

  float sSquared = sqr(s);  
  C2 = acos((sqr(LengthPsiExt) - sSquared - sqr(LengthThetaExt)) / (-2 * s * LengthThetaExt));
  B2 = acos((sqr(LengthPsi) - sSquared - sqr(LengthTheta)) / (-2 * s * LengthTheta));
  P1 = B2 + C2;
  p = sqrt(sqr(LengthTheta) + sqr(LengthThetaExt) - 2 * LengthTheta * LengthThetaExt * cos(P1));
  D = acos((sqr(LengthThetaExt) - sqr(LengthTheta) - sqr(p)) / (-2 * LengthTheta * p));
  
  // f_delta[X_AXIS] is Theta angle
  // f_delta[Y_AXIS] is Psi angle
  rho = RADIANS(f_delta[X_AXIS]) + D;

// Derived from X,Y coordinates
  delta[X_AXIS] = p * cos(rho) + SCARA_offset_x;  
  delta[Y_AXIS] = p * sin(rho) + SCARA_offset_y;  
}  

// Soft endstops on theta and psi
#define MAX_THETA 150		
#define MIN_THETA -50
#define MAX_PSI 245
#define MIN_PSI -30
#define SMALLEST_DIFFERENCE_ANGLE 30	// Smallest angle between psi and theta

void calculate_delta(float cartesian[3])
{
  // TODO Add scaling using axis_scaling[X_AXIS] and axis_scaling[Y_AXIS] to input on values to move commands (get_coordinates and all other functions manipulting destination)
  
  // SCARA "X" = theta
  // SCARA "Y" = psi+theta, motor movement inverted.

  float SCARA_pos[2], D, C1, p, rho; //, cos_rho;
  
  SCARA_pos[X_AXIS] = (cartesian[X_AXIS] - SCARA_offset_x);  // Translate SCARA to standard X Y
  SCARA_pos[Y_AXIS] = (cartesian[Y_AXIS] - SCARA_offset_y); 
  
  rho = atan2(SCARA_pos[Y_AXIS], SCARA_pos[X_AXIS]);
  
  // Avoid divide by zero
/*  cos_rho = cos(rho);
  if (cos_rho < 0.1)
    p = SCARA_pos[Y_AXIS]/sin(rho);
  else
    p = SCARA_pos[X_AXIS]/cos_rho;
  */

  float pSquared = sqr(SCARA_pos[X_AXIS]) + sqr(SCARA_pos[Y_AXIS]);
  
  p = sqrt(pSquared);
  
//  double pSquared = sqr(p);

  D = acos((sqr(LengthThetaExt) - sqr(LengthTheta) - pSquared) / (-2 * LengthTheta * p));
  C1 = acos((sqr(LengthPsiExt) - sqr(LengthPsi) - pSquared) / (-2 * LengthPsi * p));
  
  SCARA_theta = (rho - D) * SCARA_RAD2DEG - add_homeing[0];    // Convert to Angle in Degrees 
  SCARA_psi = (rho + C1) * SCARA_RAD2DEG - add_homeing[1];     // Convert to Angle in Degrees 
  
  if (SCARA_psi - SCARA_theta < SMALLEST_DIFFERENCE_ANGLE)
  {
     SERIAL_ECHOLN("angle between psi and theta too small");
     
     if ((SCARA_theta + SMALLEST_DIFFERENCE_ANGLE) < MAX_PSI)
       SCARA_psi = SCARA_theta + SMALLEST_DIFFERENCE_ANGLE;
     else if ((SCARA_psi - SMALLEST_DIFFERENCE_ANGLE) > MIN_THETA)
       SCARA_theta = SCARA_psi - SMALLEST_DIFFERENCE_ANGLE;
     else
     {
       SCARA_theta = MAX_THETA;
       SCARA_psi = SCARA_theta + SMALLEST_DIFFERENCE_ANGLE;
     }
  }
  
  if (SCARA_theta > MAX_THETA || SCARA_theta < MIN_THETA)
  {  
     SERIAL_ECHOPGM("theta out of bounds="); SERIAL_ECHOLN(SCARA_theta);
     SCARA_theta = max(MIN_THETA, SCARA_theta);
     SCARA_theta = min(MAX_THETA, SCARA_theta);
  }

  if (SCARA_psi > MAX_PSI || SCARA_psi < MIN_PSI)
  {  
     SERIAL_ECHOPGM("psi out of bounds="); SERIAL_ECHOLN(SCARA_psi);
     SCARA_psi = max(MIN_PSI, SCARA_psi);
     SCARA_psi = min(MAX_PSI, SCARA_psi);
  }
 
  delta[X_AXIS] = SCARA_theta;  
  delta[Y_AXIS] = SCARA_psi;  
  if (Y_gridcal)
  {
     delta[Z_AXIS] = cartesian[Z_AXIS];         // Ignore cartesian calculation data
  }
  else
  {
    delta[Z_AXIS] = cartesian[Z_AXIS] + calc_bed_delta(cartesian);
  }
  
  /*
  SERIAL_ECHOPGM("cartesian x="); SERIAL_ECHO(cartesian[X_AXIS]);
  SERIAL_ECHOPGM(" y="); SERIAL_ECHO(cartesian[Y_AXIS]);
  SERIAL_ECHOPGM(" z="); SERIAL_ECHOLN(cartesian[Z_AXIS]);
  
  SERIAL_ECHOPGM("scara x="); SERIAL_ECHO(SCARA_pos[X_AXIS]);
  SERIAL_ECHOPGM(" y="); SERIAL_ECHOLN(SCARA_pos[Y_AXIS]);
  
  SERIAL_ECHOPGM("delta x="); SERIAL_ECHO(delta[X_AXIS]);
  SERIAL_ECHOPGM(" y="); SERIAL_ECHO(delta[Y_AXIS]);
  SERIAL_ECHOPGM(" z="); SERIAL_ECHOLN(delta[Z_AXIS]);

  SERIAL_ECHOPGM("rho="); SERIAL_ECHO(rho);
  SERIAL_ECHOPGM(" p="); SERIAL_ECHO(p);
  SERIAL_ECHOPGM(" D="); SERIAL_ECHO(D);
  SERIAL_ECHOPGM(" C1="); SERIAL_ECHOLN(C1);
  
  SERIAL_ECHOPGM(" Theta="); SERIAL_ECHO(SCARA_theta * SCARA_RAD2DEG);
  SERIAL_ECHOPGM(" Psi="); SERIAL_ECHOLN(SCARA_psi * SCARA_RAD2DEG);
  SERIAL_ECHOLN(" ");
  */
  
  
}

float dCartX1, dCartX2, fCartX, fCartY;
int CartX, CartY;

float calc_bed_delta(float cartesian[3])
{
  if (cartesian[X_AXIS] < X_MIN_POS || 
      cartesian[Y_AXIS] < Y_MIN_POS || 
      cartesian[X_AXIS] > X_MAX_POS || 
      cartesian[Y_AXIS] > Y_MAX_POS)    // Not within grid range!
  {
     return 0; 
  }  
  
  CartX = cartesian[X_AXIS] * dCal_X_Inv;                  // Get the current sector (X)
  fCartX = ((int)cartesian[X_AXIS] * dCal_X_Inv) - CartX;                           // Find floating point

  CartY = cartesian[Y_AXIS] * dCal_Y_Inv;                  // Get the current sector (Y)
  fCartY = ((int)cartesian[Y_AXIS] * dCal_Y_Inv) - CartY;                           // Find floating point
  
  float fCartXFrom1 = (1-fCartX);
  int CartXPlus1 = CartX+1;
  int CartYPlus1 = CartY+1;
  
  dCartX1 = fCartXFrom1 * Arm_lookup[CartX][CartY]      + (fCartX) * Arm_lookup[CartXPlus1][CartY];
  dCartX2 = fCartXFrom1 * Arm_lookup[CartX][CartYPlus1] + (fCartX) * Arm_lookup[CartXPlus1][CartYPlus1];
  
  /*
  // DEBUG Stuff
  SERIAL_ECHO("dX1:");
  SERIAL_ECHO(dCartX1);
  SERIAL_ECHO("  dX2:");
  SERIAL_ECHO(dCartX2);
  SERIAL_ECHO("  fX:");
  SERIAL_ECHO(fCartX);
  SERIAL_ECHO("  fY:");
  SERIAL_ECHO(fCartY);
  SERIAL_ECHO("  Z=");
  SERIAL_ECHOLN(dCartZ);
  // ********************************************************************
  */
  
  return dCartX1 + fCartY * (dCartX2 - dCartX1);    // Calculated Z-delta   
}


void prepare_move()
{
  
  
  //destination[Z_AXIS]+= Arm_lookup[(int)destination[X_AXIS]/10][(int) destination[Y_AXIS]/10];
  //SERIAL_ECHOPGM("*"); SERIAL_ECHO(destination[Z_AXIS]);
  
  if(SoftEndsEnabled) { clamp_to_software_endstops(destination);}    
  
  //SERIAL_ECHOPGM("cartesian x="); SERIAL_ECHO(destination[X_AXIS]);
  //        SERIAL_ECHOPGM(" y="); SERIAL_ECHOLN(destination[Y_AXIS]);

  previous_millis_cmd = millis(); 

  float difference[NUM_AXIS];
  for (int8_t i=0; i < NUM_AXIS; i++) {
    difference[i] = destination[i] - current_position[i];
  }
  
  float cartesian_mm = sqrt(sq(difference[X_AXIS]) +
                            sq(difference[Y_AXIS]) +
                            sq(difference[Z_AXIS]));
  if (cartesian_mm < 0.000001) { cartesian_mm = abs(difference[E_AXIS]); }
  if (cartesian_mm < 0.000001) { return; }
  float seconds = 6000 * cartesian_mm / feedrate / feedmultiply;
  int steps = max(1, int(DELTA_SEGMENTS_PER_SECOND * seconds));
  // SERIAL_ECHOPGM("mm="); SERIAL_ECHO(cartesian_mm);
  // SERIAL_ECHOPGM(" seconds="); SERIAL_ECHO(seconds);
  // SERIAL_ECHOPGM(" steps="); SERIAL_ECHOLN(steps);
  float fraction_steps = 1.0 / float(steps);
  float fraction = fraction_steps;
  for (int s = 1; s <= steps; s++) {
    for(int8_t i=0; i < NUM_AXIS; i++) {
      destination[i] = current_position[i] + difference[i] * (fraction_steps * s);
    }
    
//    SERIAL_ECHOPGM("cartesian x="); SERIAL_ECHO(destination[X_AXIS]);
//    SERIAL_ECHOPGM(" y="); SERIAL_ECHOLN(destination[Y_AXIS]);
    
    calculate_delta(destination);
    plan_buffer_line(delta[X_AXIS], delta[Y_AXIS], delta[Z_AXIS],
                     destination[E_AXIS], feedrate*feedmultiply/60/100.0,
                     active_extruder);

    fraction += fraction_steps;
  }
  
   
  for(int8_t i=0; i < NUM_AXIS; i++) {
    current_position[i] = destination[i];
  }
}

void prepare_arc_move(char isclockwise) {
  float r = hypot(offset[X_AXIS], offset[Y_AXIS]); // Compute arc radius for mc_arc

  // Trace the arc
  mc_arc(current_position, destination, offset, X_AXIS, Y_AXIS, Z_AXIS, feedrate*feedmultiply/60/100.0, r, isclockwise, active_extruder);
  
  // As far as the parser is concerned, the position is now == target. In reality the
  // motion control system might still be processing the action and the real tool position
  // in any intermediate location.
  for(int8_t i=0; i < NUM_AXIS; i++) {
    current_position[i] = destination[i];
  }
  previous_millis_cmd = millis();
}

#ifdef CONTROLLERFAN_PIN
unsigned long lastMotor = 0; //Save the time for when a motor was turned on last
unsigned long lastMotorCheck = 0;

void controllerFan()
{
  if ((millis() - lastMotorCheck) >= 2500) //Not a time critical function, so we only check every 2500ms
  {
    lastMotorCheck = millis();
    
    if(!READ(X_ENABLE_PIN) || !READ(Y_ENABLE_PIN) || !READ(Z_ENABLE_PIN)
    #if EXTRUDERS > 2
       || !READ(E2_ENABLE_PIN)
    #endif
    #if EXTRUDER > 1
       || !READ(E2_ENABLE_PIN)
    #endif
       || !READ(E0_ENABLE_PIN)) //If any of the drivers are enabled...    
    {
      lastMotor = millis(); //... set time to NOW so the fan will turn on
    }
    
    if ((millis() - lastMotor) >= (CONTROLLERFAN_SEC*1000UL) || lastMotor == 0) //If the last time any driver was enabled, is longer since than CONTROLLERSEC...   
    {
      WRITE(CONTROLLERFAN_PIN, LOW); //... turn the fan off
    }
    else
    {
      WRITE(CONTROLLERFAN_PIN, HIGH); //... turn the fan on
    }
  }
}
#endif

void manage_inactivity() 
{ 
  if( (millis() - previous_millis_cmd) >  max_inactive_time ) 
    if(max_inactive_time) 
      kill(); 
  if(stepper_inactive_time)  {
    if( (millis() - previous_millis_cmd) >  stepper_inactive_time ) 
    {
      if(blocks_queued() == false) {
        disable_x();
        disable_y();
        disable_z();
        disable_e0();
        disable_e1();
        disable_e2();
      }
    }
  }
  #if( KILL_PIN>-1 )
    if( 0 == READ(KILL_PIN) )
      kill();
  #endif
  #ifdef CONTROLLERFAN_PIN
    controllerFan(); //Check if fan should be turned on to cool stepper drivers down
  #endif
  #ifdef EXTRUDER_RUNOUT_PREVENT
    if( (millis() - previous_millis_cmd) >  EXTRUDER_RUNOUT_SECONDS*1000 ) 
    if(degHotend(active_extruder)>EXTRUDER_RUNOUT_MINTEMP)
    {
     bool oldstatus=READ(E0_ENABLE_PIN);
     enable_e0();
     float oldepos=current_position[E_AXIS];
     float oldedes=destination[E_AXIS];
     plan_buffer_line(current_position[X_AXIS], current_position[Y_AXIS], current_position[Z_AXIS], 
                      current_position[E_AXIS]+EXTRUDER_RUNOUT_EXTRUDE*EXTRUDER_RUNOUT_ESTEPS/axis_steps_per_unit[E_AXIS], 
                      EXTRUDER_RUNOUT_SPEED/60.*EXTRUDER_RUNOUT_ESTEPS/axis_steps_per_unit[E_AXIS], active_extruder);
     current_position[E_AXIS]=oldepos;
     destination[E_AXIS]=oldedes;
     plan_set_e_position(oldepos);
     previous_millis_cmd=millis();
     st_synchronize();
     WRITE(E0_ENABLE_PIN,oldstatus);
    }
  #endif
  check_axes_activity();
}

void kill()
{
  cli(); // Stop interrupts
  disable_heater();

  disable_x();
  disable_y();
  disable_z();
  disable_e0();
  disable_e1();
  disable_e2();
  
  if(PS_ON_PIN > -1) pinMode(PS_ON_PIN,INPUT);
  SERIAL_ERROR_START;
  SERIAL_ERRORLNPGM(MSG_ERR_KILLED);
  LCD_ALERTMESSAGEPGM(MSG_KILLED);
  suicide();
  while(1) { /* Intentionally left empty */ } // Wait for reset
}

void Stop()
{
  disable_heater();
  if(Stopped == false) {
    Stopped = true;
    Stopped_gcode_LastN = gcode_LastN; // Save last g_code for restart
    SERIAL_ERROR_START;
    SERIAL_ERRORLNPGM(MSG_ERR_STOPPED);
    LCD_MESSAGEPGM(MSG_STOPPED);
  }
}

bool IsStopped() { return Stopped; };

#ifdef FAST_PWM_FAN
void setPwmFrequency(uint8_t pin, int val)
{
  val &= 0x07;
  switch(digitalPinToTimer(pin))
  {
 
    #if defined(TCCR0A)
    case TIMER0A:
    case TIMER0B:
//         TCCR0B &= ~(_BV(CS00) | _BV(CS01) | _BV(CS02));
//         TCCR0B |= val;
         break;
    #endif

    #if defined(TCCR1A)
    case TIMER1A:
    case TIMER1B:
//         TCCR1B &= ~(_BV(CS10) | _BV(CS11) | _BV(CS12));
//         TCCR1B |= val;
         break;
    #endif

    #if defined(TCCR2)
    case TIMER2:
    case TIMER2:
         TCCR2 &= ~(_BV(CS10) | _BV(CS11) | _BV(CS12));
         TCCR2 |= val;
         break;
    #endif

    #if defined(TCCR2A)
    case TIMER2A:
    case TIMER2B:
         TCCR2B &= ~(_BV(CS20) | _BV(CS21) | _BV(CS22));
         TCCR2B |= val;
         break;
    #endif

    #if defined(TCCR3A)
    case TIMER3A:
    case TIMER3B:
    case TIMER3C:
         TCCR3B &= ~(_BV(CS30) | _BV(CS31) | _BV(CS32));
         TCCR3B |= val;
         break;
    #endif

    #if defined(TCCR4A) 
    case TIMER4A:
    case TIMER4B:
    case TIMER4C:
         TCCR4B &= ~(_BV(CS40) | _BV(CS41) | _BV(CS42));
         TCCR4B |= val;
         break;
   #endif

    #if defined(TCCR5A) 
    case TIMER5A:
    case TIMER5B:
    case TIMER5C:
         TCCR5B &= ~(_BV(CS50) | _BV(CS51) | _BV(CS52));
         TCCR5B |= val;
         break;
   #endif

  }
}
#endif //FAST_PWM_FAN

bool setTargetedHotend(int code){
  tmp_extruder = active_extruder;
  if(code_seen('T')) {
    tmp_extruder = code_value();
    if(tmp_extruder >= EXTRUDERS) {
      SERIAL_ECHO_START;
      switch(code){
        case 104:
          SERIAL_ECHO(MSG_M104_INVALID_EXTRUDER);
          break;
        case 105:
          SERIAL_ECHO(MSG_M105_INVALID_EXTRUDER);
          break;
        case 109:
          SERIAL_ECHO(MSG_M109_INVALID_EXTRUDER);
          break;
      }
      SERIAL_ECHOLN(tmp_extruder);
      return true;
    }
  }
  return false;
}
