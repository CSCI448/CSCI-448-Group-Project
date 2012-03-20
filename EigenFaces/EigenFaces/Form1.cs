using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using System.IO;
using System.Runtime.Serialization;
using System.Runtime.Serialization.Formatters.Binary;

using Emgu.CV;
using Emgu.CV.CvEnum;
using Emgu.CV.Structure;
using Emgu.CV.UI;

using AForge;
using AForge.Video;
using AForge.Video.DirectShow;

using Biometrics_EmguCV;

namespace BiometricsEignefaces
{
    public partial class Form1 : Form
    {
        //Globals
        bool closing = false;
        HaarCascade haarCascade = new HaarCascade("C:/opencv/data/haarcascades/haarcascade_frontalface_alt.xml");
        EigenObjectRecognizer objRec = null;
        Stream stream;
        BinaryFormatter bformatter = new BinaryFormatter();
        Capture capture = null;
        Image<Bgr,byte> currentFrame = null;
        Image<Gray, byte> faceIso = null;
        int hcount = 0, vcount = 0;
        Image<Gray, byte>[] horizontal = new Image<Gray, byte>[40];
        Image<Gray, byte>[] vertical = new Image<Gray, byte>[40];
        private bool saving = false;
        //AForge Stuff
        bool directShow = true;
        int captureDevice = 0;
        VideoCaptureDevice videoSource = null;

        //Mark's Classes
        FaceDetector faceDetector = new FaceDetector(true);
        static int minEyeMovement = 7;
        static int deadEyeMovement = 5;
        JitterFilter leftEyeFilter = new JitterFilter(minEyeMovement, deadEyeMovement);
        JitterFilter rightEyeFilter = new JitterFilter(minEyeMovement, deadEyeMovement);
        JitterFilter faceFilter = new JitterFilter(15, 5);

        public Form1()
        {
            InitializeComponent();
        }

        private void Form1_Load(object sender, EventArgs e)
        {
            //File exists?
            if(File.Exists("EigenRecognizer.bio")){
                //Get the EigenObjectRecognizer
                stream = File.Open("EigenRecognizer.bio",FileMode.Open);
                bformatter = new BinaryFormatter();
                Console.WriteLine("Reading Eigenface Information");
                objRec = (EigenObjectRecognizer)bformatter.Deserialize(stream);
                stream.Close();
                Console.WriteLine("Finished Reading Eigenface Information");
            }
            if (!directShow)
            {
                capture = new Capture();//TODO: Exception here will skip the rest
                if (capture != null)
                {
                    //Setup webcam stream
                    Application.Idle += new EventHandler(camCapture);
                }
                else
                {
                    MessageBox.Show("No camera detected.", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                }
            }
            else
            {
                // enumerate video devices
                FilterInfoCollection videoDevices = new FilterInfoCollection(FilterCategory.VideoInputDevice);
                Console.WriteLine("vids" + videoDevices.Count);
                // create video source
                videoSource = new VideoCaptureDevice(videoDevices[captureDevice].MonikerString);
                // set NewFrame event handler
                videoSource.NewFrame += new NewFrameEventHandler(video_NewFrame);
                // start the video source
                videoSource.Start();
                Console.WriteLine("Running: " + videoSource.IsRunning.ToString());
            }
        }

        private void Form1_FormClosing(object sender, FormClosingEventArgs e)
        {
            //Serialize it
            if (objRec != null)
            {
                Console.WriteLine("Writing Eigenface Information");
                stream = File.Open("EigenRecognizer.bio", FileMode.Create);
                bformatter = new BinaryFormatter();
                bformatter.Serialize(stream, objRec);
                stream.Close();
                Console.WriteLine("Finished Writing Eigenface Information");
            }
            if (directShow)
            {
                videoSource.SignalToStop();
                videoSource = null;
            }
        }

        private void video_NewFrame(object sender, NewFrameEventArgs e)
        {
            if (!closing)
            {
                //getBackToMainThread(new Image<Bgr, byte>((Bitmap)e.Frame.Clone()));
                currentFrame = new Image<Bgr, byte>((Bitmap)e.Frame.Clone());
                //currentFrame._Flip(FLIP.HORIZONTAL);
                camCapture(null, null);
            }
        }

        //get back to main thread
        public void getBackToMainThread(Image<Bgr, byte> frame)
        {
            try
            {
                MethodInvoker method = delegate
                {
                    currentFrame = frame;
                    //currentFrame._Flip(FLIP.HORIZONTAL);
                    camCapture(null, null);
                };

                Invoke(method);
            }
            catch
            {
                //window closing
            }
        }


        public void camCapture(object sender, EventArgs e){

            //If we have to switch to DirectShow for his laptop then
            //use AForge.Net's DirectShow Stuff to get a bitmap and
            //set it into the EmguCV Image. Might be slower, but if
            //it works...
            if (!directShow)
            {
                //Capture
                currentFrame = capture.QueryFrame();
            }
            Image<Bgr, byte> tempEq = currentFrame.Copy();
            //tempEq._EqualizeHist();

            //Detect Face - Using FaceDetector class
            Face face = faceDetector.findFace(tempEq.Convert<Gray, byte>());

            leftEyeFilter.filter(ref face.leftEye);
            rightEyeFilter.filter(ref face.rightEye);
            faceFilter.filter(ref face.faceBox);

            //since we filtered the facebox, the center of the face and roll needs to be recalculated
            faceDetector.recalculate(ref face);

            if (!face.faceBox.IsEmpty)
            {
                Image<Bgr, byte> temp = currentFrame.Copy();
                if (!face.leftEye.IsEmpty && !face.rightEye.IsEmpty)
                    temp = currentFrame.Rotate(face.roll, new Bgr(0, 0, 0));
                face.faceBox.Inflate((int)((face.faceBox.Width * 0.80) - face.faceBox.Width), (int)((face.faceBox.Height * 0.90) - face.faceBox.Height));
                face.faceBox.Offset(0, 10);
                temp.ROI = face.faceBox;
                temp._EqualizeHist();
                temp.ROI = Rectangle.Empty;
                //Isolate and Correct Face
                faceIso = temp.Copy(face.faceBox).Convert<Gray, byte>();
                faceIso = faceIso.Resize(100, 100, INTER.CV_INTER_CUBIC);
                ibIso.Image = faceIso.Bitmap;
                //Draw rect on original
                currentFrame.Draw(face.faceBox, new Bgr(255, 255, 255), 2);

                recongnizeFace();
            }
            else
            {
                faceIso = null;
                ibIso.Image = null;
            }

            //display original
            ibWebcam.Image = currentFrame.Bitmap;

            //Are we saving images?
            if (saving)
            {
                saveHorizontal();
                if (!saving)
                    writeTrainingToDisk();
            }
        }

        public void recongnizeFace()
        {
            if (faceIso != null)
            {
                if (objRec != null)
                {
                    //Get the distances
                    float[] distances = objRec.GetEigenDistances(faceIso);

                    //get the labels (must copy)
                    string[] labels = new string[objRec.Labels.Length];
                    objRec.Labels.CopyTo(labels, 0);

                    Array.Sort(distances, labels); //Sort label array by distances. (Increasing Order)

                    Dictionary<string, int> votedNames = new Dictionary<string, int>();

                    //Add new name or increase the occurence of the name
                    for (int i = 0; i < 40; i++)
                    {
                        string name = labels[i];

                        if (!votedNames.ContainsKey(name))
                        {
                            //add it
                            votedNames.Add(name, 1);
                        }
                        else
                        {
                            //modify weight
                            votedNames[name] += 1;
                        }
                    }

                    //print probabilities out in order
                    List<string> results = new List<string>();
                    string[] names = votedNames.Keys.ToArray();
                    int[] weights = votedNames.Values.ToArray();
                    Array.Sort(weights, names);
                    Array.Reverse(weights);
                    Array.Reverse(names);
                    for (int i = 0; i < names.Length; i++)
                    {
                        results.Add(names[i] + " " + ((int)(((double)weights[i] / (double)40) * 100)).ToString() + "%");
                    }

                    //tbResults.Lines = results.ToArray();
                    MethodInvoker del = delegate {tbResults.Lines = results.ToArray();};
                    try
                    {
                        tbResults.Invoke(del);
                    }
                    catch
                    {
                        //window closing
                    }
                    //Console.WriteLine(results[0]);
                }
            }
            //return null;
        }

        
        public void saveHorizontal()
        {
            if (hcount < 40)
            {
                if (faceIso != null)
                {
                    //save current faceIso into array at count
                    horizontal[hcount] = faceIso;
                    Console.WriteLine(hcount + " of 39");
                    //update count
                    hcount++; //reset after 39
                }
            }
            else
            {
                saving = false;
                hcount = 0;
            }

        }

        public void saveVertical()
        {
            vcount++; //reset after 39
        }

        public void trainEigenObjectRecognizer(Image<Gray,byte>[] images, string[] names)
        {
            //This is to train from disk
            MCvTermCriteria crit = new MCvTermCriteria(50, 0.05);
            objRec = new EigenObjectRecognizer(images, names, 500, ref crit);
            Console.WriteLine("Eigen Object Recognizer Trained");
        }

        private void button1_Click(object sender, EventArgs e)
        {
            if (tbName.Text == "")
            {
                MessageBox.Show("Please enter a name for the new user.", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
            else
            {
                //Give instructions
                //Start recording
                saving = true;
            }
        }

        private void writeTrainingToDisk()
        {
            if (!horizontal.Contains<Image<Gray, byte>>(null))
            {
                //Check if face folder is there...
                if (!Directory.Exists("trainingImages"))
                {
                    //Make it if not
                    Directory.CreateDirectory("trainingImages");
                    Console.WriteLine("Created directory for training images");
                }
                
                Console.WriteLine("Directory for training images exists");
                if (!Directory.Exists("trainingImages\\" + tbName.Text))
                    Directory.CreateDirectory("trainingImages\\" + tbName.Text);
                //write images 0-39
                string path = "";
                for (int i = 0; i < 40; i++)
                {
                    //append name to path for new folder
                    path = "trainingImages\\" + tbName.Text + "\\" + i.ToString() + ".bmp";
                    horizontal[i].Save(path);
                }
                Console.WriteLine("Finished writing training images");

                MethodInvoker del = delegate { tbName.Text = ""; };
                tbName.Invoke(del);
                

                readTrainingFromDisk();
            }
            else
            {
                MessageBox.Show("The image array contains null images","Error",MessageBoxButtons.OK,MessageBoxIcon.Error);
            }
        }

        private void readTrainingFromDisk()
        {
            //Check for folder
            if (Directory.Exists("trainingImages"))
            {
                //TODO: Allow different numbers of pictures

                //Make array of names from folder names
                string[] names = Directory.GetDirectories("trainingImages");

                //remove svn folder
                List<string> tempNames = new List<string>();
                foreach (string name in names)
                {
                    if (name != "trainingImages\\.svn"){
                        tempNames.Add(name);
                    }
                }
                names = tempNames.ToArray();

                //Make array of images from images in folders
                string[] imageNames = new string[names.Length * 40];
                string[] fullNames = new string[names.Length * 40]; // Array with repeated names, one for each picture
                string[] tempImages;
                int count = 0;
                bool error = false;
                string errorName = "";
                foreach (string name in names)
                {
                    tempImages = Directory.GetFiles(name);

                    if (tempImages.Length < 40) {
                        error = true;
                        errorName = name;
                        break;
                    }

                    tempImages.CopyTo(imageNames,count*40);
                    //Make names have the same name for each file of that person;
                    for (int i = 0; i < 40; i++)
                    {
                        fullNames[i + (count * 40)] = name.Replace("trainingImages\\", "");
                    }

                    count++;
                }
                if (!error)
                {
                    //Create images for each image and put in array
                    Image<Gray, byte>[] images = new Image<Gray, byte>[names.Length * 40]; //40 Images per person
                    count = 0;
                    foreach (string file in imageNames)
                    {
                        images[count] = new Image<Gray, byte>(file);
                        count++;
                    }
                    trainEigenObjectRecognizer(images, fullNames);
                }
                else
                {
                    MessageBox.Show("Insifficient Images for "+ errorName, "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                }
            }
            else //error, we cant find the folder
            {
                MessageBox.Show("Cannot find Training Images.", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }
    }
}
