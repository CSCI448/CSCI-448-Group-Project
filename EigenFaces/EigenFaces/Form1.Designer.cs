namespace BiometricsEignefaces
{
    partial class Form1
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.label1 = new System.Windows.Forms.Label();
            this.label2 = new System.Windows.Forms.Label();
            this.button1 = new System.Windows.Forms.Button();
            this.label3 = new System.Windows.Forms.Label();
            this.tbResults = new System.Windows.Forms.TextBox();
            this.tbName = new System.Windows.Forms.TextBox();
            this.label4 = new System.Windows.Forms.Label();
            this.ibWebcam = new System.Windows.Forms.PictureBox();
            this.ibIso = new System.Windows.Forms.PictureBox();
            ((System.ComponentModel.ISupportInitialize)(this.ibWebcam)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.ibIso)).BeginInit();
            this.SuspendLayout();
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(12, 9);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(86, 13);
            this.label1.TabIndex = 3;
            this.label1.Text = "Webcam Stream";
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(365, 9);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(141, 13);
            this.label2.TabIndex = 4;
            this.label2.Text = "Isolated and Equalized Face";
            // 
            // button1
            // 
            this.button1.Location = new System.Drawing.Point(232, 271);
            this.button1.Name = "button1";
            this.button1.Size = new System.Drawing.Size(100, 23);
            this.button1.TabIndex = 5;
            this.button1.Text = "Enroll New User";
            this.button1.UseVisualStyleBackColor = true;
            this.button1.Click += new System.EventHandler(this.button1_Click);
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.label3.Location = new System.Drawing.Point(414, 128);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(42, 13);
            this.label3.TabIndex = 6;
            this.label3.Text = "Results";
            // 
            // tbResults
            // 
            this.tbResults.Location = new System.Drawing.Point(347, 144);
            this.tbResults.Multiline = true;
            this.tbResults.Name = "tbResults";
            this.tbResults.ReadOnly = true;
            this.tbResults.ScrollBars = System.Windows.Forms.ScrollBars.Vertical;
            this.tbResults.Size = new System.Drawing.Size(183, 150);
            this.tbResults.TabIndex = 7;
            this.tbResults.WordWrap = false;
            // 
            // tbName
            // 
            this.tbName.Location = new System.Drawing.Point(106, 273);
            this.tbName.Name = "tbName";
            this.tbName.Size = new System.Drawing.Size(120, 20);
            this.tbName.TabIndex = 8;
            // 
            // label4
            // 
            this.label4.AutoSize = true;
            this.label4.Location = new System.Drawing.Point(12, 276);
            this.label4.Name = "label4";
            this.label4.Size = new System.Drawing.Size(88, 13);
            this.label4.TabIndex = 9;
            this.label4.Text = "New User Name:";
            // 
            // ibWebcam
            // 
            this.ibWebcam.Location = new System.Drawing.Point(15, 25);
            this.ibWebcam.Name = "ibWebcam";
            this.ibWebcam.Size = new System.Drawing.Size(320, 240);
            this.ibWebcam.SizeMode = System.Windows.Forms.PictureBoxSizeMode.StretchImage;
            this.ibWebcam.TabIndex = 10;
            this.ibWebcam.TabStop = false;
            // 
            // ibIso
            // 
            this.ibIso.Location = new System.Drawing.Point(385, 25);
            this.ibIso.Name = "ibIso";
            this.ibIso.Size = new System.Drawing.Size(100, 100);
            this.ibIso.TabIndex = 11;
            this.ibIso.TabStop = false;
            // 
            // Form1
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(545, 304);
            this.Controls.Add(this.ibIso);
            this.Controls.Add(this.ibWebcam);
            this.Controls.Add(this.label4);
            this.Controls.Add(this.tbName);
            this.Controls.Add(this.tbResults);
            this.Controls.Add(this.label3);
            this.Controls.Add(this.button1);
            this.Controls.Add(this.label2);
            this.Controls.Add(this.label1);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedSingle;
            this.MaximizeBox = false;
            this.Name = "Form1";
            this.Text = "Face Detection and Recognition";
            this.FormClosing += new System.Windows.Forms.FormClosingEventHandler(this.Form1_FormClosing);
            this.Load += new System.EventHandler(this.Form1_Load);
            ((System.ComponentModel.ISupportInitialize)(this.ibWebcam)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.ibIso)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.Button button1;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.TextBox tbResults;
        private System.Windows.Forms.TextBox tbName;
        private System.Windows.Forms.Label label4;
        private System.Windows.Forms.PictureBox ibWebcam;
        private System.Windows.Forms.PictureBox ibIso;
    }
}

