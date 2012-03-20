using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Drawing;

namespace Biometrics_EmguCV
{
    class JitterFilter
    {
        int px = -1;
        int py = -1;
        int ph = -1;
        int pw = -1;

        int minMovement;
        int deadMovement;

        public JitterFilter(int min, int dead)
        {
            minMovement = min;
            deadMovement = dead;
        }

        public void filter(ref Point point){
            if (point.IsEmpty)
                return;

            int x = point.X;
            int y = point.Y;

            double distanceMoved = Math.Sqrt(Math.Pow(x-px,2) + Math.Pow(y-py,2));

            if(distanceMoved < deadMovement){
                point.X = px;
                point.Y = py;
                return;
            }
            else if (distanceMoved < minMovement)
            {
                double proportion = Math.Pow((1.0 / (double)minMovement) * distanceMoved, 2);

                //Console.WriteLine(distanceMoved + " " + proportion);

                //similar triangles
                point.X = (int)(px + (x - px) * proportion);
                point.Y = (int)(py + (y - py) * proportion);

                //return;
            }

            px = point.X;
            py = point.Y;
        }

        public void filter(ref Rectangle rectangle)
        {
            if (rectangle.IsEmpty)
                return;

            //filter the xy
            Point center = rectangle.Location;
            filter(ref center);
            rectangle.X = center.X;
            rectangle.Y = center.Y;

            //filter the height
            int heightChange = Math.Abs(rectangle.Height - ph);
            if (heightChange < deadMovement)
            {
                rectangle.Height = ph;
            }
            else if (heightChange < minMovement)
            {
                double proportion = Math.Pow((1.0 / (double)minMovement) * heightChange, 2);

                //Console.WriteLine(heightChange + " " + proportion);

                rectangle.Height = (int)(ph + (rectangle.Height - ph) * proportion);
            }

            if (heightChange >= deadMovement)
            {
                ph = rectangle.Height;
            }

            //filter the width
            int widthChange = Math.Abs(rectangle.Width - pw);
            if (widthChange < deadMovement)
            {
                rectangle.Width = pw;
            }
            else if (widthChange < minMovement)
            {
                double proportion = Math.Pow((1.0 / (double)minMovement) * widthChange, 2);

                //Console.WriteLine(heightChange + " " + proportion);

                //similar triangles
                rectangle.Width = (int)(pw + (rectangle.Width - pw) * proportion);
            }

            if (widthChange >= deadMovement)
            {
                pw = rectangle.Width;
            }
        }

    }
}
