/****************************************************************************************************

RepRapFirmware - G Codes

This class interprets G Codes from one or more sources, and calls the functions in Move, Heat etc
that drive the machine to do what the G Codes command.

-----------------------------------------------------------------------------------------------------

Version 0.1

13 February 2013

Adrian Bowyer
RepRap Professional Ltd
http://reprappro.com

Licence: GPL

****************************************************************************************************/

#ifndef GCODES_H
#define GCODES_H

#define STACK 5
#define GCODE_LENGTH 100 // Maximum length of internally-generated G Code string

#define AXIS_LETTERS { 'X', 'Y', 'Z' } // The axes in a GCode
#define FEEDRATE_LETTER 'F'// GCode feedrate
#define EXTRUDE_LETTER 'E' // GCode extrude

// Small class to hold an individual GCode and provide functions to allow it to be parsed

class GCodeBuffer
{
  public:
    GCodeBuffer(Platform* p, const char* id);
    void Init(); 										// Set it up
    bool Put(char c);									// Add a character to the end
    bool Seen(char c);									// Is a character present?
    float GetFValue();									// Get a float after a key letter
    int GetIValue();									// Get an integer after a key letter
    long GetLValue();									// Get a long integer after a key letter
    const char* GetUnprecedentedString();				// Get a string with no preceeding key letter
    const char* GetString();							// Get a string after a key letter
    const void GetFloatArray(float a[], int& length);	// Get a :-separated list of floats after a key letter
    const void GetLongArray(long l[], int& length);		// Get a :-separated list of longs after a key letter
    const char* Buffer();								// All of the G Code itself
    bool Finished() const;								// Has the G Code been executed?
    void SetFinished(bool f);							// Set the G Code executed (or not)
    const char* WritingFileDirectory() const;			// If we are writing the G Code to a file, where that file is
    void SetWritingFileDirectory(const char* wfd);		// Set the directory for the file to write the GCode in
    
  private:
    int CheckSum();										// Compute the checksum (if any) at the end of the G Code
    Platform* platform;									// Pointer to the RepRap's controlling class
    char gcodeBuffer[GCODE_LENGTH];						// The G Code
    const char* identity;								// Where we are from (web, file, serial line etc)
    int gcodePointer;									// Index in the buffer
    int readPointer;									// Where in the buffer to read next
    bool inComment;										// Are we after a ';' character?
    bool finished;										// Has the G Code been executed?
    const char* writingFileDirectory;					// If the G Code is going into a file, where that is
};

//****************************************************************************************************

// The GCode interpreter

class GCodes
{   
  public:
  
    GCodes(Platform* p, Webserver* w);
    void Spin();														// Called in a tight loop to make this class work
    void Init();														// Set it up
    void Exit();														// Shut it down
    bool RunConfigurationGCodes();										// Run the configuration G Code file on reboot
    bool ReadMove(float* m, bool& ce);									// Called by the Move class to get a movement set by the last G Code
    void QueueFileToPrint(const char* fileName);						// Open a file of G Codes to run
    void DeleteFile(const char* fileName);								// Does what it says
    bool GetProbeCoordinates(int count, float& x, float& y, float& z);	// Get pre-recorded probe coordinates
    char* GetCurrentCoordinates();										// Get where we are as a string
    float FractionOfFilePrinted() const;								// Are we in the middle of printing a file? -ve means no, value is fraction printed
    void Diagnostics();													// Send helpful information out
    bool HaveIncomingData() const;										// Is there something that we have to do?
    bool GetAxisHasBeenHomed(uint8_t axis) const { return axisHasBeenHomed[axis]; } // Is the axis at 0?
    
  private:
  
    void DoFilePrint(GCodeBuffer* gb);									// Get G Codes from a file and print them
    bool AllMovesAreFinishedAndMoveBufferIsLoaded();					// Wait for move queue to exhaust and the current position is loaded
    bool DoCannedCycleMove(bool ce);									// Do a move from an internally programmed canned cycle
    bool DoFileMacro(const char* fileName);						// Run a GCode macro in a file
    bool FileCannedCyclesReturn();										// End a macro
    bool ActOnCode(GCodeBuffer* gb);									// Do a G, M or T Code
    bool HandleGcode(int code, GCodeBuffer* gb);						// Do a G Code
    bool HandleMcode(int code, GCodeBuffer* gb);						// Do an M Code
    bool HandleTcode(int code, GCodeBuffer* gb);						// Do a T Code
    bool SetUpMove(GCodeBuffer* gb);									// Set up a new movement
    bool DoDwell(GCodeBuffer *gb);										// Wait for a bit
    bool DoHome(char *reply, bool& error);								// Home some axes
    bool DoSingleZProbeAtPoint();										// Probe at a given point
    bool DoSingleZProbe();												// Probe where we are
    bool SetSingleZProbeAtAPosition(GCodeBuffer *gb, char *reply);		// Probes at a given position - see the comment at the head of the function itself
    //bool DoMultipleZProbe(char* reply);									// Probes a series of points and sets the bed equation
    bool SetBedEquationWithProbe();											// Probes a series of points and sets the bed equation
    bool SetPrintZProbe(GCodeBuffer *gb, char *reply);					// Either return the probe value, or set its threshold
    void SetOrReportOffsets(char* reply, GCodeBuffer *gb);				// Deal with a G10
    bool SetPositions(GCodeBuffer *gb);									// Deal with a G92
    bool LoadMoveBufferFromGCode(GCodeBuffer *gb,  						// Set up a move for the Move class
    		bool doingG92, bool applyLimits);
    bool NoHome() const;												// Are we homing and not finished?
    bool Push();														// Push feedrate etc on the stack
    bool Pop();															// Pop feedrate etc
    bool DisableDrives();												// Turn the motors off
    bool StandbyHeaters();												// Set all heaters to standby temperatures
    void SetEthernetAddress(GCodeBuffer *gb, int mCode);				// Does what it says
    void SetMACAddress(GCodeBuffer *gb);								// Deals with an M540
    void HandleReply(bool error, bool fromLine, const char* reply, 		// If the GCode is from the serial interface, reply to it
    		char gMOrT, int code, bool resend);
    void OpenFileToWrite(const char* directory,							// Start saving GCodes in a file
    		const char* fileName, GCodeBuffer *gb);
    void WriteGCodeToFile(GCodeBuffer *gb);								// Write this GCode into a file
    bool SendConfigToLine();											// Deal with M503
    void WriteHTMLToFile(char b, GCodeBuffer *gb);						// Save an HTML file (usually to upload a new web interface)
    bool OffsetAxes(GCodeBuffer *gb);									// Set offsets - deprecated, use G10
    int8_t Heater(int8_t head) const;									// Legacy G codes start heaters at 0, but we use 0 for the bed.  This sorts that out.
    void AddNewTool(GCodeBuffer *gb, char* reply);						// Create a new tool definition
    void SetToolHeaters(float temperature);								// Set all a tool's heaters to the temperature.  For M104...
    bool ChangeTool(int newToolNumber);									// Select a new tool

    Platform* platform;							// The RepRap machine
    bool active;								// Live and running?
    Webserver* webserver;						// The webserver class
    float dwellTime;							// How long a pause for a dwell (seconds)?
    bool dwellWaiting;							// We are in a dwell
    GCodeBuffer* webGCode;						// The sources...
    GCodeBuffer* fileGCode;						// ...
    GCodeBuffer* serialGCode;					// ...
    GCodeBuffer* fileMacroGCode;				// ... of G Codes
    bool moveAvailable;							// Have we seen a move G Code and set it up?
    float moveBuffer[DRIVES+1]; 				// Move coordinates; last is feed rate
    bool checkEndStops;							// Should we check them on the next move?
    bool drivesRelative; 						// Are movements relative - all except X, Y and Z
    bool axesRelative;   						// Are movements relative - X, Y and Z
    bool drivesRelativeStack[STACK];			// For dealing with Push and Pop
    bool axesRelativeStack[STACK];				// For dealing with Push and Pop
    float feedrateStack[STACK];					// For dealing with Push and Pop
    FileStore* fileStack[STACK];				// For dealing with Push and Pop
    int8_t stackPointer;						// Push and Pop stack pointer
    char axisLetters[AXES]; 					// 'X', 'Y', 'Z'
    float lastPos[DRIVES - AXES]; 				// Just needed for relative moves; i.e. not X, Y and Z
	float record[DRIVES+1];						// Temporary store for move positions
	float moveToDo[DRIVES+1];					// Where to go set by G1 etc
	bool activeDrive[DRIVES+1];					// Is this drive involved in a move?
	bool offSetSet;								// Are any axis offsets non-zero?
    float distanceScale;						// MM or inches
    FileStore* fileBeingPrinted;				// The file being printed at the moment (if any)
    FileStore* fileToPrint;						// A file to print in the future, or one that has been paused
    FileStore* fileBeingWritten;				// A file to write G Codes (or sometimes HTML) in
    FileStore* configFile;						// A file containing a macro
    float fractionOfFilePrinted;				// Only used to record the main file when a macro is being printed
    bool doingFileMacro;						// Are we executing a macro file?
    char* eofString;							// What's at the end of an HTML file?
    uint8_t eofStringCounter;					// Check the...
    uint8_t eofStringLength;					// ... EoF string as we read.
    //int8_t selectedHead;						// Which extruder is in use
    bool homeX;									// True to home the X axis this move
    bool homeY;									// True to home the Y axis this move
    bool homeZ;									// True to home the Z axis this move
    int8_t homeAxisMoveCount;					// Counts homing moves
    int probeCount;								// Counts multiple probe points
    int8_t cannedCycleMoveCount;				// Counts through internal (i.e. not macro) canned cycle moves
    bool cannedCycleMoveQueued;					// True if a canned cycle move has been set
    bool zProbesSet;							// True if all Z probing is done and we can set the bed equation
    float longWait;								// Timer for things that happen occasionally (seconds)
    bool limitAxes;								// Don't think outside the box.
    bool axisHasBeenHomed[3];						// These record which of the axes have been homed
    int8_t toolChangeSequence;					// Steps through the tool change procedure
};

//*****************************************************************************************************

// Get an Int after a G Code letter

inline int GCodeBuffer::GetIValue()
{
  return (int)GetLValue();
}

inline const char* GCodeBuffer::Buffer()
{
  return gcodeBuffer;
}

inline bool GCodeBuffer::Finished() const
{
  return finished;
}

inline void GCodeBuffer::SetFinished(bool f)
{
  finished = f;
}

inline const char* GCodeBuffer::WritingFileDirectory() const
{
	return writingFileDirectory;
}

inline void GCodeBuffer::SetWritingFileDirectory(const char* wfd)
{
	writingFileDirectory = wfd;
}

inline float GCodes::FractionOfFilePrinted() const
{
  if(fileBeingPrinted == NULL)
	  	 return -1.0;
  if(fractionOfFilePrinted < 0.0)
	  return fileBeingPrinted->FractionRead();
  return fractionOfFilePrinted;
}

inline bool GCodes::HaveIncomingData() const
{
	return fileBeingPrinted != NULL || webserver->GCodeAvailable() || (platform->GetLine()->Status() & byteAvailable);
}

inline bool GCodes::NoHome() const
{
   return !(homeX || homeY || homeZ || homeAxisMoveCount);
}

// This function takes care of the fact that the heater and head indices 
// don't match because the bed is heater 0.

inline int8_t GCodes::Heater(int8_t head) const
{
   return head+1; 
}

// Run the configuration G Code file to set up the machine.  Usually just called once
// on re-boot.

inline bool GCodes::RunConfigurationGCodes()
{
	return !DoFileMacro(platform->GetConfigFile());
}

//inline int8_t GCodes::GetSelectedHead()
//{
//  return selectedHead;
//}

#endif
