// *****************************************************************************
//
//      Copyright (c) 2013, Pleora Technologies Inc., All rights reserved.
//
// *****************************************************************************

//
// Shows how to use a PvStream object to acquire images from a GigE Vision or
// USB3 Vision device.
//
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <iostream>
using namespace cv;
using namespace std;
#include <string>
#include <sstream>
#include <PvSampleUtils.h>
#include <PvDevice.h>
#include <PvDeviceGEV.h>
#include <PvDeviceU3V.h>
#include <PvStream.h>
#include <PvStreamGEV.h>
#include <PvStreamU3V.h>
#include <PvBuffer.h>

#include "Ljack.h"
#include "Ljack_DAC.h"
#include "macros.h"
#include "vectra.h"
#include "string.h"
#include "vChronos.h"
#include <boost/thread.hpp>
#include <boost/date_time.hpp>
//#include "vPlotterT.h"

//#include "vImg.hpp"

#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <math.h>
#include <assert.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

#define PV_GUI_NOT_AVAILABLE
#ifdef PV_GUI_NOT_AVAILABLE
#include <PvSystem.h>
#else
#include <PvDeviceFinderWnd.h>
#endif // PV_GUI_NOT_AVAILABLE

#include <list>
typedef std::list<PvBuffer *> BufferList;
float tiptilt_factor = 0.25;
float flower_x(float t);
float flower_y(float t);


PV_INIT_SIGNAL_HANDLER();

#define BUFFER_COUNT ( 16 )

///
/// Function Prototypes
///
#ifdef PV_GUI_NOT_AVAILABLE
const PvDeviceInfo *SelectDevice( PvSystem *aPvSystem );
#else
const PvDeviceInfo *SelectDevice( PvDeviceFinderWnd *aDeviceFinderWnd );
#endif // PV_GUI_NOT_AVAILABLE
PvDevice *ConnectToDevice( const PvDeviceInfo *aDeviceInfo );
PvStream *OpenStream( const PvDeviceInfo *aDeviceInfo );
void ConfigureStream( PvDevice *aDevice, PvStream *aStream );
void CreateStreamBuffers( PvDevice *aDevice, PvStream *aStream, BufferList *aBufferList );
void AcquireImages( PvDevice *aDevice, PvStream *aStream );
void FreeStreamBuffers( BufferList *aBufferList );
string IntToString ( int number );

//
// Main function
//
int main()
{

    const PvDeviceInfo *lDeviceInfo = NULL;
    PvDevice *lDevice = NULL;
    PvStream *lStream = NULL;
    BufferList lBufferList;

    PV_SAMPLE_INIT();

#ifdef PV_GUI_NOT_AVAILABLE
    PvSystem *lPvSystem = new PvSystem;
    lDeviceInfo = SelectDevice( lPvSystem );
#else
    PvDeviceFinderWnd *lDeviceFinderWnd = new PvDeviceFinderWnd();
    lDeviceInfo = SelectDevice( lDeviceFinderWnd );
#endif // PV_GUI_NOT_AVAILABLE

    cout << "PvStreamSample:" << endl << endl;

    if ( NULL != lDeviceInfo )
    {
        lDevice = ConnectToDevice( lDeviceInfo );
        if ( NULL != lDevice )
        {
            lStream = OpenStream( lDeviceInfo );
            if ( NULL != lStream )
            {
                ConfigureStream( lDevice, lStream );
                CreateStreamBuffers( lDevice, lStream, &lBufferList );
                AcquireImages( lDevice, lStream );

                FreeStreamBuffers( &lBufferList );

                // Close the stream
                cout << "Closing stream" << endl;
                lStream->Close();
                PvStream::Free( lStream );
            }

            // Disconnect the device
            cout << "Disconnecting device" << endl;
            lDevice->Disconnect();
            PvDevice::Free( lDevice );
        }
    }

    cout << endl;
    cout << "<press a key to exit>" << endl;
    PvWaitForKeyPress();

#ifdef PV_GUI_NOT_AVAILABLE
    if( NULL != lPvSystem )
    {
        delete lPvSystem;
        lPvSystem = NULL;
    }
#else
    if( NULL != lDeviceFinderWnd )
    {
        delete lDeviceFinderWnd;
        lDeviceFinderWnd = NULL;
    }
#endif // PV_GUI_NOT_AVAILABLE

    PV_SAMPLE_TERMINATE();

    return 0;
}





///Fonctions

#ifdef PV_GUI_NOT_AVAILABLE
const PvDeviceInfo *SelectDevice( PvSystem *aPvSystem )
{
    const PvDeviceInfo *lDeviceInfo = NULL;
    PvResult lResult;

    if (NULL != aPvSystem)
    {
        // Get the selected device information.
        lDeviceInfo = PvSelectDevice( *aPvSystem );
    }

    return lDeviceInfo;
}
#else
const PvDeviceInfo *SelectDevice( PvDeviceFinderWnd *aDeviceFinderWnd )
{
    const PvDeviceInfo *lDeviceInfo = NULL;
    PvResult lResult;

    if (NULL != aDeviceFinderWnd)
    {
        // Display list of GigE Vision and USB3 Vision devices
        lResult = aDeviceFinderWnd->ShowModal();
        if ( !lResult.IsOK() )
        {
            // User hit cancel
            cout << "No device selected." << endl;
            return NULL;
        }

        // Get the selected device information.
        lDeviceInfo = aDeviceFinderWnd->GetSelected();
    }

    return lDeviceInfo;
}
#endif // PV_GUI_NOT_AVAILABLE

PvDevice *ConnectToDevice( const PvDeviceInfo *aDeviceInfo )
{
    PvDevice *lDevice;
    PvResult lResult;

    // Connect to the GigE Vision or USB3 Vision device
    cout << "Connecting to " << aDeviceInfo->GetDisplayID().GetAscii() << "." << endl;
    lDevice = PvDevice::CreateAndConnect( aDeviceInfo, &lResult );
    if ( lDevice == NULL )
    {
        cout << "Unable to connect to " << aDeviceInfo->GetDisplayID().GetAscii() << "." << endl;
    }

    return lDevice;
}

PvStream *OpenStream( const PvDeviceInfo *aDeviceInfo )
{
    PvStream *lStream;
    PvResult lResult;

    // Open stream to the GigE Vision or USB3 Vision device
    cout << "Opening stream to device." << endl;
    lStream = PvStream::CreateAndOpen( aDeviceInfo->GetConnectionID(), &lResult );
    if ( lStream == NULL )
    {
        cout << "Unable to stream from " << aDeviceInfo->GetDisplayID().GetAscii() << "." << endl;
    }

    return lStream;
}

void ConfigureStream( PvDevice *aDevice, PvStream *aStream )
{
    // If this is a GigE Vision device, configure GigE Vision specific streaming parameters
    PvDeviceGEV* lDeviceGEV = dynamic_cast<PvDeviceGEV *>( aDevice );
    if ( lDeviceGEV != NULL )
    {
        PvStreamGEV *lStreamGEV = static_cast<PvStreamGEV *>( aStream );

        // Negotiate packet size
        lDeviceGEV->NegotiatePacketSize();

        // Configure device streaming destination
        lDeviceGEV->SetStreamDestination( lStreamGEV->GetLocalIPAddress(), lStreamGEV->GetLocalPort() );
    }
}

void CreateStreamBuffers( PvDevice *aDevice, PvStream *aStream, BufferList *aBufferList )
{
    // Reading payload size from device
    uint32_t lSize = aDevice->GetPayloadSize();

    // Use BUFFER_COUNT or the maximum number of buffers, whichever is smaller
    uint32_t lBufferCount = ( aStream->GetQueuedBufferMaximum() < BUFFER_COUNT ) ?
        aStream->GetQueuedBufferMaximum() :
        BUFFER_COUNT;

    // Allocate buffers
    for ( uint32_t i = 0; i < lBufferCount; i++ )
    {
        // Create new buffer object
        PvBuffer *lBuffer = new PvBuffer;

        // Have the new buffer object allocate payload memory
        lBuffer->Alloc( static_cast<uint32_t>( lSize ) );

        // Add to external list - used to eventually release the buffers
        aBufferList->push_back( lBuffer );
    }

    // Queue all buffers in the stream
    BufferList::iterator lIt = aBufferList->begin();
    while ( lIt != aBufferList->end() )
    {
        aStream->QueueBuffer( *lIt );
        lIt++;
    }
}

void AcquireImages( PvDevice *aDevice, PvStream *aStream )
{
    Mat image;

    // Get device parameters need to control streaming
    PvGenParameterArray *lDeviceParams = aDevice->GetParameters();

    // Map the GenICam AcquisitionStart and AcquisitionStop commands
    PvGenCommand *lStart = dynamic_cast<PvGenCommand *>( lDeviceParams->Get( "AcquisitionStart" ) );
    PvGenCommand *lStop = dynamic_cast<PvGenCommand *>( lDeviceParams->Get( "AcquisitionStop" ) );

    // Get stream parameters
    PvGenParameterArray *lStreamParams = aStream->GetParameters();

    // Map a few GenICam stream stats counters
    PvGenFloat *lFrameRate = dynamic_cast<PvGenFloat *>( lStreamParams->Get( "AcquisitionRate" ) );
    PvGenFloat *lBandwidth = dynamic_cast<PvGenFloat *>( lStreamParams->Get( "Bandwidth" ) );

    // Enable streaming and send the AcquisitionStart command
    cout << "Enabling streaming and sending AcquisitionStart command." << endl;
    aDevice->StreamEnable();
    lStart->Execute();

    char lDoodle[] = "|\\-|-/";
    int lDoodleIndex = 0;
    double lFrameRateVal = 0.0;
    double lBandwidthVal = 0.0;
    int cpt=0;
    // Acquire images until the user instructs us to stop.
    cout << endl << "<press a key to stop streaming>" << endl;


       ////////////////////////////////////////////////////////////////////////////////////////
    /// BOUCLE ACQUISITION

    // synchro instrumentale:
    boost::posix_time::milliseconds delay( 3.4 ); //1.2 + 1.2 +1 );
    boost::posix_time::milliseconds delay_tilt( 1.5 );

    vChronos vTime("prise d'images"); vTime.clear();

    vTime.start();    //while ( !PvKbHit() )//tant que touche pas pressée

    for(short int numImg=0;numImg<20;numImg++ )
    {
        PvBuffer *lBuffer = NULL;
        PvResult lOperationResult;




        // Retrieve next buffer
        PvResult lResult = aStream->RetrieveBuffer( &lBuffer, &lOperationResult, 1000 );
        if ( lResult.IsOK() )///Buffer acquis?
        {
            if ( lOperationResult.IsOK() ) ///l'opération finie sans erreur? (time out..)
            {
                PvPayloadType lType;

                //
                // We now have a valid buffer. This is where you would typically process the buffer.
                // -----------------------------------------------------------------------------------------
                // ...

                lFrameRate->GetValue( lFrameRateVal );
                lBandwidth->GetValue( lBandwidthVal );

                // If the buffer contains an image, display width and height.
                uint32_t lWidth = 0, lHeight = 0;
                lType = lBuffer->GetPayloadType();

                cout << fixed << setprecision( 1 );
                cout << lDoodle[ lDoodleIndex ];
                cout << " BlockID: " << uppercase << hex << setfill( '0' ) << setw( 16 ) << lBuffer->GetBlockID();


                if ( lType == PvPayloadTypeImage )///le buffer contient bien une image? Tout est ok, on traite le flux
                {   cpt++;//incrémenter le numero de l'image

                    // Get image specific buffer interface.
                    PvImage *lImage = lBuffer->GetImage();

                    // Get image data pointer so we can pass it to CV::MAT container
                    unsigned char *img = lImage->GetDataPointer();


                    // Read width, height.
                    lWidth = lImage->GetWidth();
                    lHeight = lImage->GetHeight();
                    cout << "  W: " << dec << lWidth << " H: " << lHeight;

                    // Copy/convert Pleora Vision image pointer to cv::Mat container
                    cv::Mat lframe(lHeight,lWidth,CV_8UC1,img, cv::Mat::AUTO_STEP);
                   // namedWindow( "Display window", WINDOW_AUTOSIZE );// Create a window for display.
                    //imshow("Display window",lframe);
                    string racine="/home/mat/frame";
                    string ext=".pgm";
                    string num = IntToString ( cpt );
                    string nom_final=racine+num+ext;
                    //cout<<nom_final<<endl;
                    imwrite(nom_final, lframe);
                   /* if(waitKey(3) >= 0)
                    {
                        destroyAllWindows() ;
                        break;
                    }*/
                    //waitKey(0);
                    //imwrite( "/home/mat/test_camera.pgm", lframe );
                }
                else
                {
                    cout << " (buffer does not contain image)";
                }
                cout << "  " << lFrameRateVal << " FPS  " << ( lBandwidthVal / 1000000.0 ) << " Mb/s   \r";
            }
            else
            {
                // Non OK operational result
                cout << lDoodle[ lDoodleIndex ] << " " << lOperationResult.GetCodeString().GetAscii() << "\r";
            }

            // Re-queue the buffer in the stream object
            aStream->QueueBuffer( lBuffer );
        }
        else
        {
            // Retrieve buffer failure
            cout << lDoodle[ lDoodleIndex ] << " " << lResult.GetCodeString().GetAscii() << "\r";
        }

        ++lDoodleIndex %= 6;
    }

    PvGetChar(); // Flush key buffer for next stop.
    cout << endl << endl;

    // Tell the device to stop sending images.
    cout << "Sending AcquisitionStop command to the device" << endl;
    lStop->Execute();

    // Disable streaming on the device
    cout << "Disable streaming on the controller." << endl;
    aDevice->StreamDisable();

    // Abort all buffers from the stream and dequeue
    cout << "Aborting buffers still in stream" << endl;
    aStream->AbortQueuedBuffers();
    while ( aStream->GetQueuedBufferCount() > 0 )
    {
        PvBuffer *lBuffer = NULL;
        PvResult lOperationResult;

        aStream->RetrieveBuffer( &lBuffer, &lOperationResult );
    }
}

void FreeStreamBuffers( BufferList *aBufferList )
{
    // Go through the buffer list
    BufferList::iterator lIt = aBufferList->begin();
    while ( lIt != aBufferList->end() )
    {
        delete *lIt;
        lIt++;
    }

    // Clear the buffer list
    aBufferList->clear();
}

std::string IntToString ( int number )
{
  std::ostringstream oss;

  // Works just like cout
  oss<< number;

  // Return the underlying string
  return oss.str();
}
