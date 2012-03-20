using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Drawing;

using Emgu.CV;
using Emgu.CV.Structure;

namespace Biometrics_EmguCV
{
    //this struct holds the location of the face and eyes in an image, plus its roll relative to the cammera
    struct Face
    {
        public Rectangle faceBox;
        public Point faceCenter;
        public Point leftEye;
        public Point rightEye;
        public double roll;
    }

    //this class finds a face in a frame and returns its location in a Face struct
    class FaceDetector
    {
        private Rectangle prevFaceROI; //where the face was found previously
        private bool isPrevious = false; //if there was a face found previously
        private bool doPreviousSearch = true; //whether to use the previous location of the face or not

        //the haar cascades
        private HaarCascade faceHaar = new HaarCascade("C:/opencv/data/haarcascades/haarcascade_frontalface_alt.xml");
        private HaarCascade eyeHaar = new HaarCascade("C:/opencv/data/haarcascades/haarcascade_eye.xml");

        //constructor that sets whether or not to use the previous location of the face
        //set this to false if the face will be fast moving, or detections don't occur on sequential frames
        //set to true for a 5-6 fps boost, and graceful performance degradation as face distance increases
        public FaceDetector(bool previousSearch)
        {
            doPreviousSearch = previousSearch;
        }

        //sets whether or not to use the previous location of the face
        //set this to false if the face will be fast moving, or detections don't occur on sequential frames
        //set to true for a 5-6 fps boost, and graceful performance degradation as face distance increases
        public void setPreviousSearch(bool true_false){
            doPreviousSearch = true_false;
        }

        //pass this the frame to look for a face in
        //returns a Face struct with the detected face, eyes, and roll
        public Face findFace(Emgu.CV.Image<Gray, byte> frame)
        {
            Face face = new Face();
            face.faceBox = findFaceBox(frame);

            if (!face.faceBox.IsEmpty)
            {
                face.faceCenter = new Point(face.faceBox.X + face.faceBox.Width / 2, face.faceBox.Y + face.faceBox.Height / 2);
            }
            else
            {
                face.faceCenter = Point.Empty;
            }
            
            face.leftEye = findEye(face.faceBox, 0, frame);
            face.rightEye = findEye(face.faceBox, 1, frame);

            face.roll = getRoll(face.leftEye, face.rightEye);

            return face;
        }

        public void recalculate(ref Face face){
            if (!face.faceBox.IsEmpty)
            {
                face.faceCenter.X = face.faceBox.X + face.faceBox.Width / 2;
                face.faceCenter.Y = face.faceBox.Y + face.faceBox.Height / 2;
            }

            if (!face.leftEye.IsEmpty && !face.rightEye.IsEmpty)
                face.roll = getRoll(face.leftEye, face.rightEye);
        }

        private Rectangle findFaceBox(Emgu.CV.Image<Gray, byte> frame)
        {
            //minimum face size to look for
            System.Drawing.Size minFace;

            //we can use a previous location of the face to speed up detection
            if (isPrevious)
            {
                //only look for faces that are at least 0.25 of the previously found face
                minFace = new System.Drawing.Size(prevFaceROI.Width - (int)(prevFaceROI.Width * 0.25), prevFaceROI.Height - (int)(prevFaceROI.Height * 0.25));

                //look for the face in an area around the previous face that has been inflated by 0.25 of the size of the previous face
                prevFaceROI.Inflate((int)(prevFaceROI.Width * 0.25), (int)(prevFaceROI.Height * 0.25));

                //set the search rectangle
                frame.ROI = prevFaceROI;
            }
            else
            {
                //no previously detected faces so search the whole frame for faces at least 100x100
                frame.ROI = Rectangle.Empty;
                minFace = new System.Drawing.Size(100, 100);
            }

            //use viola-jones to find the face
            Emgu.CV.Structure.MCvAvgComp[][] faces = frame.DetectHaarCascade(faceHaar, 1.2, 4, Emgu.CV.CvEnum.HAAR_DETECTION_TYPE.FIND_BIGGEST_OBJECT | Emgu.CV.CvEnum.HAAR_DETECTION_TYPE.DO_ROUGH_SEARCH, minFace);
            if (!(faces[0].Length > 0))
            {
                //no face detected
                isPrevious = false;
                frame.ROI = Rectangle.Empty;
                return Rectangle.Empty;
            }

            Rectangle returnRectangle = faces[0][0].rect;

            //if the area the face was found in the previous frame was used, then the cordinates of the face need to be made global
            if (isPrevious)
            {
                returnRectangle.X += frame.ROI.X;
                returnRectangle.Y += frame.ROI.Y;
            }

            //save the face rectangle for starting the search on the next frame
            if (doPreviousSearch)
                isPrevious = true;
            prevFaceROI = returnRectangle;

            frame.ROI = Rectangle.Empty;
            return returnRectangle;
        }

        //eyenum: 0 for left, 1 for right
        private Point findEye(Rectangle faceROI, int eyeNum, Emgu.CV.Image<Gray, byte> frame)
        {
            //return invalid rectangle if the face is an invalid rectangle
            if (faceROI.IsEmpty)
            {
                return Point.Empty;
            }

            frame.ROI = Rectangle.Empty;

            Rectangle eyeROI = faceROI;
            eyeROI.Width = eyeROI.Width / 2;
            eyeROI.Y = eyeROI.Y + eyeROI.Height / 5;
            eyeROI.Height = eyeROI.Height - (int)((double)eyeROI.Height / 1.6);
            if (eyeNum == 1)
            {
                eyeROI.X += faceROI.Width / 2;
            }

            System.Drawing.Size minEye = new System.Drawing.Size(15, 15);

            frame.ROI = eyeROI;
            Emgu.CV.Structure.MCvAvgComp[][] eyes = frame.DetectHaarCascade(eyeHaar, 1.1, 4, Emgu.CV.CvEnum.HAAR_DETECTION_TYPE.FIND_BIGGEST_OBJECT | Emgu.CV.CvEnum.HAAR_DETECTION_TYPE.DO_ROUGH_SEARCH, minEye);
            if (!(eyes[0].Length > 0))
            {
                frame.ROI = Rectangle.Empty;
                return Point.Empty;
            }
            eyes[0][0].rect.X += eyeROI.X;
            eyes[0][0].rect.Y += eyeROI.Y;

            Point returnPoint = new Point(eyes[0][0].rect.X + eyes[0][0].rect.Width / 2, eyes[0][0].rect.Y + eyes[0][0].rect.Height / 2);

            frame.ROI = Rectangle.Empty;
            return returnPoint;
        }

        private double getRoll(Point lEye, Point rEye)
        {
            int oposite = lEye.Y - rEye.Y;
            int adjacent = rEye.X -lEye.X;

            if (adjacent == 0)
            {
                return 90.0; //assume positive 90 degrees
            }
            else
            {
                return Math.Atan((double)oposite / (double)adjacent) * 180 / Math.PI;
            }
        }

    }

}
