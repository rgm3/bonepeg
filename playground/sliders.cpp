/**
 * Reading and setting settings from the camera.
 *
 * Doesn't work with MBP FaceTimeHD or Logitech C920 on OSX :(
 *
 * http://answers.opencv.org/question/30811/what-are-the-default-webcam-settings/?answer=30817#post-id-30817
 */
#include <iostream>
#include <opencv2/opencv.hpp>

using namespace std;
using namespace cv;

float Brightness;
float Contrast ;
float Saturation;
float Gain;

int B;
int C;
int S;
int G;

char winName[20]="Live";
Mat frame;
VideoCapture cap(0);

void onTrackbar_changed(int, void*)
{
 Brightness =float(B)/100;
 Contrast   =float(C)/100;
 Saturation =float(S)/100;
 Gain       =float(G)/100;

 cout << "bright " << Brightness << endl;

bool result = cap.set(CV_CAP_PROP_BRIGHTNESS,Brightness);
cout << "      result: " << result << endl;
cap.set(CV_CAP_PROP_CONTRAST, Contrast);
cap.set(CV_CAP_PROP_SATURATION, Saturation);
cap.set(CV_CAP_PROP_GAIN, Gain);

}

int main(int, char**)
{
  cap.set(CV_CAP_PROP_FRAME_WIDTH, 640);
  cap.set(CV_CAP_PROP_FRAME_HEIGHT, 480);

    if(!cap.isOpened())  // check if we succeeded
    {
        cout << "Camera open fail." << endl;
        return -1;
    }

    cout<<"Press 's' to save snapshot, ESC to quit."<<endl;
 namedWindow(winName);

 Brightness = cap.get(CV_CAP_PROP_BRIGHTNESS);
 Contrast   = cap.get(CV_CAP_PROP_CONTRAST );
 Saturation = cap.get(CV_CAP_PROP_SATURATION);
 Gain       = cap.get(CV_CAP_PROP_GAIN);

 cout<<"===================================="<<endl<<endl;
 cout<<"Default Brightness -------> "<<Brightness<<endl;
 cout<<"Default Contrast----------> "<<Contrast<<endl;
 cout<<"Default Saturation--------> "<<Saturation<<endl;
 cout<<"Default Gain--------------> "<<Gain<<endl<<endl;
 cout<<"===================================="<<endl;

  B=int(Brightness*100);
  C=int(Contrast*100);
  S=int(Saturation*100);
  G=int(Gain*100);

createTrackbar( "Brightness",winName, &B, 100, onTrackbar_changed );
createTrackbar( "Contrast",winName, &C, 100,onTrackbar_changed );
createTrackbar( "Saturation",winName, &S, 100,onTrackbar_changed);
createTrackbar( "Gain",winName, &G, 100,onTrackbar_changed);

    int i=0;
    char name[10];
    for(;;)
    {

        cap >> frame; // get a new frame from camera
        imshow(winName, frame);
        char c=waitKey(30);

        if(c=='s') {
     sprintf(name,"%d.jpg",i++);
     imwrite(name,frame);
    }
        if( c== 27) break;
    }
return 0;
}
