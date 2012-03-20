/*
   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "asmmodel.h"
#include "util.h"
#include <cstdio>

int burrito = 0;
int framers = 0;
string outputtxt3 = "";
vector< Point_<int> > V2;
vector< Point_<int> > V3;
vector< Point_<int> > V4;
vector< Point_<int> > V5;
vector< Point_<int> > VC;
vector< Point_<int> > VA;
vector< Point_<int> > VAP;
vector< Point_<int> > VNeutral;
Mat mbp;
Mat mbpp;
Mat mbppp;
float eyeBR4518;
float eyeBL3724;
float mouth5751;
float eyeR3335;
float eyeL2830;
int threshhold = 1;
float BEN[10] = {109.041276588,111.0720486891,28.0713376952,16.2788205961,
	16.0312195419,29.0172362571,12.0415945788,32.9848450049,33.2415402772,43.0116263352};
float NAIM[10] = {103.0776406404,103.121287812,25.079872408,12.1655250606,
	12.0415945788,24.1867732449,12.1655250606,33.8378486314,34.0147027034,45.0444225182};
float KOSSI[10] = {90.1387818866,90.0499861188,18.0277563773,9,9.0553851381,
	20.0249843945,14.0356688476,32.0156211872,32.5576411922,32.0624390838};
float JOHN[10] = {125.1439171514,125.0359948175,29.0688837075,11.0453610172,
	12.0415945788,28.0713376952,17.0293863659,28.6356421266,29.1547594742,50.0099990002};
float current[10] = {0};

void ASMModel::buildModel()
{
    ShapeModel::buildModel();
    
    printf("Building active model...\n");
    buildLocalDiffStructure();
}

void ASMModel::buildLocalDiffStructure()
{
    int i, j, l;
    // First, we have find a proper "step" based on the size of face
    int xMin, xMax, yMin, yMax;
    vector< double > myStep;
    myStep.resize(nTrain);
    for (i=0; i<nTrain; i++){
        xMin = yMin = 100000000;
        xMax = yMax = 0;
        for (j=0; j<nMarkPoints; j++){
            if (imageSet[i].points[j].x < xMin)
                xMin = imageSet[i].points[j].x;
            if (imageSet[i].points[j].y < yMin)
                yMin = imageSet[i].points[j].y;
            if (imageSet[i].points[j].x > xMax)
                xMax = imageSet[i].points[j].x;
            if (imageSet[i].points[j].y > yMax)
                yMax = imageSet[i].points[j].y;
        }
        myStep[i] = 1.3* sqrt( (xMax-xMin)*(yMax-yMin) / 10000.);
//         printf("step: %f\n", myStep[i]);
    }
    
    Mat_< double > *tCovar, *tMean;
    Mat_< double > datMat(2*localFeatureRad+1, nTrain);
    meanG.resize(this->pyramidLevel + 1);
    iCovarG.resize(this->pyramidLevel + 1);
    for (l = 0; l <= pyramidLevel; l++){
        for (i = 0; i<nMarkPoints; i++){
            tCovar = new Mat_< double >;
            tMean = new Mat_< double >;
            for (j = 0; j<nTrain; j++){
                Mat_<double> M;
                M = datMat.col(j);
                imageSet[j].getLocalStruct(i,localFeatureRad,l, myStep[j]).copyTo(M);
            }
            cv::calcCovarMatrix(datMat, *tCovar, *tMean,
                                CV_COVAR_NORMAL|CV_COVAR_COLS);
            *tCovar = tCovar->inv(cv::DECOMP_SVD);
            this->iCovarG[l].push_back(*tCovar);
            this->meanG[l].push_back(*tMean);
            delete tMean;
            delete tCovar;
        }
    }
}

void ASMModel::findParamForShapeBTSM(const ShapeVec &Y, const ShapeVec &Y_old,
                               FitResult & fitResult, FitResult &b_old, int l)
{
    //const double c[3] = {0.005, 0.005, 0.001};
    const double c[3] = {0.0005, 0.0005, 0.0005};
    
    double rho2, delta2, x2;
    double p;
    ShapeVec y_r, y_rpr, xFromParam, xFromY, x;
    
    ShapeVec yt = Y_old;
    yt -= Y;
    rho2 = c[l] * yt.dot(yt);
    
    SimilarityTrans curTrans = b_old.transformation;
    Mat_< double > curParam, tmpFullParam, lastParam;
    
    curParam.create(pcaPyr[l].eigenvalues.rows, 1);
    for (int i=0; i<pcaPyr[l].eigenvalues.rows; i++)
        if (i<b_old.params.rows)
            curParam(i, 0) = b_old.params(i, 0);
        else
            curParam(i, 0) = 0;
    //curParam = curParam.rowRange(0, pcaPyr[l].eigenvalues.rows);
    
    int ii=0;
    do{
        double s = curTrans.getS();
        lastParam = curParam.clone();
        
        // Expectation Step
        curTrans.backTransform(Y, y_r);
        p = sigma2Pyr[l]/(sigma2Pyr[l] + rho2/(s * s));
        //printf("p: %g, rho2/s2: %g, sigma2: %g\n", p, rho2/(s * s), sigma2Pyr[l]);
        delta2 = 1/(1/sigma2Pyr[l] + s*s / rho2);
//         printf("p: %g, rho2/s2: %g, sigma2: %g, delta2: %g\n", 
//                p, rho2/(s * s), sigma2Pyr[l], delta2);
        
        this->pcaPyr[l].backProject(curParam, xFromParam);
        
        pcaFullShape->project(y_r, tmpFullParam);
        pcaFullShape->backProject(tmpFullParam, y_rpr);
        x = p*y_rpr + (1-p) * xFromParam;
        x2 = x.dot(x) + (x.rows - 4) * delta2;
//         printf("p: %g, rho2/s2: %g, sigma2: %g, delta2: %g, x.x: %g, x2: %g\n", 
//                p, rho2/(s * s), sigma2Pyr[l], delta2, x.dot(x), x2);
        
        // Maximization Step
        pcaPyr[l].project(x, curParam);
        
        for (int i=0; i<pcaPyr[l].eigenvalues.rows; i++)
            curParam(i, 0) *= (pcaShape->eigenvalues.at<double>(i, 0))/
                            (pcaShape->eigenvalues.at<double>(i, 0)+sigma2Pyr[l]);
        int nP = x.rows / 2;
        curTrans.a = Y.dot(x) / x2;
        curTrans.b = 0;
        for (int i=0; i<nP; i++)
            curTrans.b += x.X(i) * Y.Y(i) - x.Y(i)*Y.X(i);
        curTrans.b /= x2;
        curTrans.Xt = Y.getXMean();
        curTrans.Yt = Y.getYMean();
        
        //clampParamVec(curParam);
        ii++;
    } while (norm(lastParam-curParam)>1e-4 && ii<20);
    fitResult.params = curParam;
    fitResult.transformation = curTrans;
}

void ASMModel::findParamForShape(const ShapeVec &Y, FitResult & fitResult){
    ShapeVec x, y;
    
    // Step 1: Init to zeros
    fitResult.params = Mat_<double>::zeros(nShapeParams, 1);
    SimilarityTrans &st = fitResult.transformation;
    
    Mat_<double> resOld;
    do {
        resOld = fitResult.params.clone();
        // Step 2: Project to x
        projectParamToShape(fitResult.params, x);
        // Step 3: Align x to Y
        st.setTransformByAlign(x, Y);
        // Step 4: Invert transform Y to y
        st.backTransform(Y, y);
        // Step 5: Align to mean shape
        y.alignTo(meanShape);
        
        // Step 6: Update parameters
        projectShapeToParam(y, fitResult.params);
        
        // Step 7:
        clampParamVec(fitResult.params);
    } while (norm(resOld-fitResult.params)>1e-3);
}

void ASMModel::fit(const Mat & img, vector< FitResult > & fitResult, 
             cv::CascadeClassifier &faceCascade, bool biggestOnly, int verbose)
{
	int cd=0;
	if (biggestOnly)
		cd = CV_HAAR_FIND_BIGGEST_OBJECT;
    // Step 1: Search for faces
    vector<cv::Rect> faces;
    try{
        faceCascade.detectMultiScale( img, faces,
            1.2, 2, cd
            //|CV_HAAR_FIND_BIGGEST_OBJECT
            //|CV_HAAR_DO_ROUGH_SEARCH
            |CV_HAAR_SCALE_IMAGE
            , Size(60, 60) );
    }
    catch (cv::Exception e){
        printf("Face detect error...(%s)\n", e.err.data());
        faces.clear();
    }
    fitResult.resize(faces.size());
    for (uint i=0; i<faces.size(); i++){
        cv::Rect r = faces[i];
        r.y -= r.height*searchYOffset;
        r.x -= r.width*searchXOffset;
        if (r.x<0) r.x = 0;
        if (r.y<0) r.y = 0;
        r.width *= searchWScale;
        r.height *= searchHScale;
        if (r.x+r.width>img.cols) r.width = img.cols-r.x;
        if (r.y+r.height>img.rows) r.height = img.rows-r.y;
        // Do the search!
        doSearch(img(r), fitResult[i], verbose);
        SimilarityTrans s2;
        s2.Xt = r.x;
        s2.Yt = r.y;
        s2.a = 1;
        fitResult[i].transformation = s2 * fitResult[i].transformation;
    }
    if (faces.size()==0){
        //printf("No face found!\n");
		framers = 0;
    }
}

cv::Mat ASMModel::showResult(Mat& img, vector< FitResult >& res)
{
    Mat mb;
	string outputtxt = "";
	string outputtxt2 = "Not moving: X";
	string outputtxt4 = "";
	string outputtxt5 = "";
	string outputtxt6 = "Not moving: Y";
    if (img.channels()==1)
        cv::cvtColor(img, mb, CV_GRAY2RGB);
    else
        mb = img.clone();

	ShapeVec sv;
    if (framers <= 5) {
		 for (uint i=0; i<res.size(); i++){
			vector< Point_<int> > V;
			resultToPointList(res[i], V);
			//cout << V << "\n";
			V2 = V;
			V3 = V;
			V4 = V;
			V5 = V;
			VC = V;
			VA = V;
			VAP = V;
			drawMarkPointsOnImg(mb, V, shapeInfo, true);
			//cout << framers << "\n";
		 }
		 mbp = mb;
		 mbpp = mb;
	}
	if (framers > 5) {
		for (uint i=0; i<res.size(); i++){
			vector< Point_<int> > V;
			resultToPointList(res[i], V);
			VC = V;
			//cout << V << "\n";
			for (int j=0; j<V.size(); j++){
				//cout << V[j].x << " " << V[j].y << "\n";
				V[j].x = (V[j].x + V2[j].x + V3[j].x + V4[j].x + V5[j].x + VA[j].x + VA[j].x)/7;
				V[j].y = (V[j].y + V2[j].y + V3[j].y + V4[j].y + V5[j].y + VA[j].y + VA[j].y)/7;
				//cout << V[j].x << " " << V[j].y << "\n";
			}
			drawMarkPointsOnImg(mbp, V, shapeInfo, true);
			V5 = V4;
			V4 = V3;
			V3 = V2;
			V2 = VC;
			VAP = VA;
			VA = V;
		}
	}

	if (framers < 10) {
		outputtxt3 = "maintain nuetral face";
		outputtxt2 = "do not move face";
		outputtxt6 = "do not move face";
		VNeutral = VA;
	} else if (framers >= 10 && framers <= 15) {
		outputtxt3 = "establishing neutral face";
		outputtxt2 = "do not move face";
		outputtxt6 = "do not move face";
		for (int j=0; j<VNeutral.size(); j++){
			VNeutral[j].x = (VNeutral[j].x + VA[j].x + VC[j].x)/3;
			VNeutral[j].y = (VNeutral[j].y + VA[j].y + VC[j].y)/3;
		}
	} else if (framers == 16) {
		
		outputtxt3 = "neutral face established";
		outputtxt2 = "do not move face";
		outputtxt6 = "do not move face";
		//cout << VNeutral;
		eyeBR4518 = (VNeutral[45].y - VNeutral[18].y);
		eyeBL3724 = (VNeutral[37].y - VNeutral[24].y);
		mouth5751 = (VNeutral[57].y - VNeutral[51].y);
		eyeR3335 = (VNeutral[35].y - VNeutral[33].y);
		eyeL2830 = (VNeutral[30].y - VNeutral[28].y);

		float x1 = pow((float)(VNeutral[0].x - VNeutral[14].x),2);
		float y1 = pow((float)(VNeutral[0].y - VNeutral[14].y),2);
		float temp = (x1 + y1);
		current[0] = sqrt(temp);
		float ZBEN = current[0]/BEN[0];
		float ZNAIM = current[0]/NAIM[0];
		float ZKOSSI = current[0]/KOSSI[0];
		float ZJOHN = current[0]/JOHN[0];

		float x2 = pow((float)(VNeutral[1].x - VNeutral[13].x),2);
		float y2 = pow((float)(VNeutral[1].y - VNeutral[13].y),2);
		temp = (x2 + y2);
		current[1] = sqrt(temp);

		float x3 = pow((float)(VNeutral[27].x - VNeutral[29].x),2);
		float y3 = pow((float)(VNeutral[27].y - VNeutral[29].y),2);
		temp = (x3 + y3);
		current[2] = sqrt(temp);

		float x4 = pow((float)(VNeutral[28].x - VNeutral[30].x),2);
		float y4 = pow((float)(VNeutral[28].y - VNeutral[30].y),2);
		temp = (x4 + y4);
		current[3] = sqrt(temp);

		float x5 = pow((float)(VNeutral[33].x - VNeutral[35].x),2);
		float y5 = pow((float)(VNeutral[33].y - VNeutral[35].y),2);
		temp = (x5 + y5);
		current[4] = sqrt(temp);

		float x6 = pow((float)(VNeutral[34].x - VNeutral[32].x),2);
		float y6 = pow((float)(VNeutral[34].y - VNeutral[32].y),2);
		temp = (x6 + y6);
		current[5] = sqrt(temp);

		float x7 = pow((float)(VNeutral[37].x - VNeutral[45].x),2);
		float y7 = pow((float)(VNeutral[37].y - VNeutral[45].y),2);
		temp = (x7 + y7);
		current[6] = sqrt(temp);

		float x8 = pow((float)(VNeutral[37].x - VNeutral[41].x),2);
		float y8 = pow((float)(VNeutral[37].y - VNeutral[41].y),2);
		temp = (x8 + y8);
		current[7] = sqrt(temp);

		float x9 = pow((float)(VNeutral[45].x - VNeutral[41].x),2);
		float y9 = pow((float)(VNeutral[45].y - VNeutral[41].y),2);
		temp = (x9 + y9);
		current[8] = sqrt(temp);

		float x10 = pow((float)(VNeutral[48].x - VNeutral[54].x),2);
		float y10 = pow((float)(VNeutral[48].y - VNeutral[54].y),2);
		temp = (x10 + y10);
		current[9] = sqrt(temp);
		
		int isBen = 0;
		int isNaim = 0;
		int isKossi = 0;
		int isJohn = 0;
		float valueB = 0;
		float valueN = 0;
		float valueK = 0;
		float valueJ = 0;
		for (int q=0;q<9;q++){
			valueB = ZBEN*BEN[q] - current[q];
			valueN = ZNAIM*NAIM[q] - current[q];
			valueK = ZKOSSI*KOSSI[q] - current[q];
			valueJ = ZJOHN*JOHN[q] - current[q];
			if (valueB < 4 && valueB > -4) {
				isBen++;
			}
			if (valueN < 4 && valueN > -4) {
				isNaim++;
			}
			if (valueK < 4 && valueK > -4) {
				isKossi++;
			}
			if (valueJ < 4 && valueJ > -4) {
				isJohn++;
			}
		}
		float thresh = (float)threshhold/100 + .5;
		int guess = 6 + thresh*2;
		cout << guess << "/n";
		if (isBen >= guess && isBen > isNaim && isBen > isKossi && isBen > isJohn){
			outputtxt3 = "Hi Ben!";
		} else if (isNaim >= guess && isNaim > isBen && isNaim > isKossi && isNaim > isJohn){
			outputtxt3 = "Hi Naim!";
		} else if (isKossi >= guess && isKossi > isBen && isKossi > isNaim && isKossi > isJohn){
			outputtxt3 = "Hi Kossi!";
		} else if (isJohn >= guess && isJohn > isBen && isJohn > isNaim && isJohn > isKossi) {
			outputtxt3 = "Hi John!";
		} else {
			outputtxt3 = "Identity Unkown";
		}
	}
	
	if (framers >= 17){
		float eyeBRC4518 = VA[45].y - VA[18].y;
		float eyeBLC3724 = VA[37].y - VA[24].y;
		float mouthC5751 = VA[57].y - VA[51].y;
		float XMove = (float)((VAP[0].x - VA[0].x) + (VAP[37].x - VA[37].x) + (VAP[45].x - VA[45].x) + (VAP[14].x - VA[14].x))/4;
		float YMove = (float)((VAP[0].y - VA[0].y) + (VAP[37].y - VA[37].y) + (VAP[45].y - VA[45].y) + (VAP[14].y - VA[14].y))/4;
		float eyeRC3335 = VA[35].y - VA[33].y;
		float eyeLC2830 = VA[30].y - VA[28].y;
		float thresh = (float)threshhold/100 + .5;
		if (eyeBRC4518 > eyeBR4518*thresh){
			outputtxt = outputtxt + "Right eyebrow raised | ";
		}
		if (eyeBLC3724 > eyeBL3724*thresh){
			outputtxt = outputtxt + "Left eyebrow raised";
		}
		if (mouthC5751 > mouth5751*thresh){
			outputtxt4 = "mouth open";
		}
		if (XMove > thresh*2) {
			outputtxt2 = "Moved Left";
		}
		if (XMove < -(thresh*2)) {
			outputtxt2 = "Moved Right";
		}
		if (YMove > thresh*2) {
			outputtxt6 = "Moved Up";
		}
		if (YMove < -(thresh*2)) {
			outputtxt6 = "Moved Down";
		}
		if (eyeRC3335 < eyeR3335/(thresh*1.25)){
			outputtxt5 = outputtxt5 + "right eye closed | ";
		}
		if (eyeLC2830 < eyeL2830/(thresh*1.25)){
			outputtxt5 = outputtxt5 + "left eye closed";
		}
	}


    if (!img.empty())
		putText(mb, outputtxt, Point(5, mb.rows-3), cv::FONT_HERSHEY_SIMPLEX, .4, cv::Scalar(150,255,76), 1, 8);
		putText(mb, outputtxt4, Point(5, mb.rows-23), cv::FONT_HERSHEY_SIMPLEX, .4, cv::Scalar(150,255,76), 1, 8);
		putText(mb, outputtxt5, Point(5, mb.rows-43), cv::FONT_HERSHEY_SIMPLEX, .4, cv::Scalar(255,150,76), 1, 8);
		putText(mb, outputtxt2, Point(5, 10), cv::FONT_HERSHEY_SIMPLEX, .4, cv::Scalar(150,255,76), 1, 8);
		putText(mb, outputtxt6, Point(mb.cols-110, 10), cv::FONT_HERSHEY_SIMPLEX, .4, cv::Scalar(150,76,255), 1, 8);
		putText(mb, outputtxt3, Point(20, 30), cv::FONT_HERSHEY_SIMPLEX, .7, cv::Scalar(50,200,76), 2, 8);
		outputtxt.clear();
        imshow("hoho", mbp);
		cv::createTrackbar("threshhold","hoho", &threshhold, 100); 

	//mbppp = mbpp;
	mbpp = mbp;
	mbp = mb;
	return mbpp;
}

void ASMModel::resultToPointList(const FitResult& fitResult, vector< Point_<int> >& pV)
{
    ShapeVec sv;
    projectParamToShape(fitResult.params, sv);
    sv.restoreToPointList(pV, fitResult.transformation, 1000);
}

void ASMModel::doSearch(const cv::Mat& img, FitResult& fitResult, int verbose)
{
    // Step 2: Ensure it is a grayscale image
    Mat grayImg;
    if (img.channels() == 3){
        cv::cvtColor(img, grayImg, CV_BGR2GRAY);
    }
    else
        grayImg = img;
    
    // Step 3: Resize each face image
    Mat resizedImg;
    // Resize the image to proper size
    double ratio;
    ratio = sqrt( double(40000) / (grayImg.rows * grayImg.cols));
    cv::resize(grayImg, resizedImg, Size(grayImg.cols*ratio, grayImg.rows*ratio));
    
    //puts("Start fitting...");
	framers++;
    ModelImage curSearch;
    curSearch.setShapeInfo( &shapeInfo );
    curSearch.loadTrainImage(resizedImg);

    fitResult.params = Mat_<double>::zeros(nShapeParams, 1);

    ShapeVec &sv = curSearch.shapeVec;
    ShapeVec shape_old;
    
    projectParamToShape(fitResult.params, sv);
    SimilarityTrans st = sv.getShapeTransformFitingSize(resizedImg.size(), 
                                                      searchScaleRatio,
                                                      searchInitXOffset,
                                                      searchInitYOffset);
    fitResult.transformation = st;
    curSearch.buildFromShapeVec(st, 1000);

    pyramidLevel = 2;
    int k=localFeatureRad;
    
    ns=4;
    
    // sum of offsets of current iteration.
    int totalOffset;
    if (verbose >= ASM_FIT_VERBOSE_AT_LEVEL)
        curSearch.show();
    // Update each point
    vector< Point_< int > > V;
    for (int l=this->pyramidLevel; l>=0; l--){
        if (verbose >= ASM_FIT_VERBOSE_AT_LEVEL)
            printf("Level %d\n", l);
        Mat_<double> img=curSearch.getDerivImage(l);
//         printf("Image size: %dx%d\n", img.cols, img.rows);
        // at most 5 iterations for each level
        int runT;
        double avgMov;
        for (runT=0; runT<10; runT++){
            // Backup current shape
            shape_old.fromPointList(curSearch.points);
            
            totalOffset = 0;
            vector< Point_< int > > bestEP(nMarkPoints);
            for (int i=0; i<this->nMarkPoints; i++){
                if (verbose >= ASM_FIT_VERBOSE_AT_POINT)
                    printf("Dealing point %d...\n", i);

                Mat_< double > nrmV(2*k+1, 1);
                double curBest=-1, ct;
                int bestI = 0;
                double absSum;
                for (int e=ns; e>=-ns; e--){
                    curSearch.getPointsOnNorm(i, k, l, V, 2*searchStepAreaRatio, e);
                
                    absSum = 0;
                    for (int j=-k;j<=k;j++){
                        nrmV(j+k, 0) = img(V[j+k]);
                        absSum += fabs(nrmV(j+k, 0));
                    }
                    nrmV *= 1/absSum;
                    ct = cv::Mahalanobis(nrmV, this->meanG[l][i], this->iCovarG[l][i]);
//                     printf("absSum: %lf, ct: %lf\n", absSum, ct);
                    if (verbose >= ASM_FIT_VERBOSE_AT_POINT)
                        curSearch.show(l, i, true, e);
                    
                    if (ct<curBest || curBest<0){
                        curBest = ct;
                        bestI = e;
                        bestEP[i] = V[k];
                    }
                }
//                 printf("best e: %d\n", bestI);
//                 bestEP[i] = V[bestI+(ns+k)];
                totalOffset += abs(bestI);
                
                if (verbose >= ASM_FIT_VERBOSE_AT_POINT)
                    curSearch.show(l, i, true, bestI);
            }
            for (int i=0;i<nMarkPoints;i++){
                curSearch.points[i] = bestEP[i];
                curSearch.points[i].x <<= l;
                if (l>0) curSearch.points[i].x += (1<<(l-1));
                curSearch.points[i].y <<= l;
                if (l>0) curSearch.points[i].y += (1<<(l-1));
            }
            curSearch.shapeVec.fromPointList(curSearch.points);
            
            if (verbose >= ASM_FIT_VERBOSE_AT_ITERATION)
                curSearch.show(l);
            
            // Project to PCA model and then back
            //findParamForShape(curSearch.shapeVec,  fitResult);
            findParamForShapeBTSM(curSearch.shapeVec, shape_old, fitResult, fitResult, l);
            
            pcaPyr[l].backProject(fitResult.params, sv);
            
            // Reconstruct new shape
            curSearch.buildFromShapeVec(fitResult.transformation, burrito);
			burrito++;
            
            avgMov = (double)totalOffset/nMarkPoints;
            if (verbose >= ASM_FIT_VERBOSE_AT_ITERATION){
                printf("Iter %d:  Average offset: %.3f\n", runT+1, avgMov);
                curSearch.show(l);
            }
            
            if (avgMov < 1.3){
                runT++;
                break;
            }
        }
        if (verbose == ASM_FIT_VERBOSE_AT_LEVEL){
            printf("%d iterations. average offset for last iter: %.3f\n", runT, avgMov);
            curSearch.show(l);
        }
    }
    
    SimilarityTrans s2;
    s2.a = 1/ratio;
    fitResult.transformation = s2 * fitResult.transformation;
}

void ASMModel::loadFromFile(ModelFile& file)
{
    ShapeModel::loadFromFile(file);
    printf("Loading ASM model from file...\n");
    //! parameter k for ASM
    file.readInt(localFeatureRad);
    file.readInt(ns);
    
    int i,j;
    int rows, cols;
    file.readInt(rows);
    file.readInt(cols);
    iCovarG.resize(pyramidLevel+1);
    for (i=0;i<=pyramidLevel;i++){
        iCovarG[i].resize(nMarkPoints);
        for (j=0;j<nMarkPoints;j++){
            iCovarG[i][j].create(rows, cols);
            for (int ii=0;ii<rows;ii++)
                for (int jj=0;jj<cols;jj++)
                    file.readReal(iCovarG[i][j](ii, jj));
        }
    }
    
    file.readInt(rows);
    file.readInt(cols);
    meanG.resize(pyramidLevel+1);
    for (i=0;i<=pyramidLevel;i++){
        meanG[i].resize(nMarkPoints);
        for (j=0;j<nMarkPoints;j++){
            meanG[i][j].create(rows, cols);
            for (int ii=0;ii<rows;ii++)
                for (int jj=0;jj<cols;jj++)
                    file.readReal(meanG[i][j](ii, jj));
        }
    }
    
    // Prepare BTSM pyramid
    double curSigma2 = 0;
    for (i=0; i<pcaFullShape->eigenvalues.rows; i++){
        curSigma2 += pcaFullShape->eigenvalues.at<double>(i, 0);
    }
    printf("sssssssssig: %g\n", curSigma2);
    
    // Layer 2, 5 parameter
    for (i=0; i<5 && i<pcaFullShape->eigenvalues.rows; i++){
        curSigma2 -= pcaFullShape->eigenvalues.at<double>(i, 0);
    }
    sigma2Pyr[2] = curSigma2 / (nMarkPoints*2-4);
    printf("sssssssssig: %g\n", curSigma2);
    pcaPyr[2].eigenvalues = pcaFullShape->eigenvalues.rowRange(0, i);
    pcaPyr[2].eigenvectors = pcaFullShape->eigenvectors.rowRange(0, i);
    pcaPyr[2].mean = pcaFullShape->mean;
    
    // Layer 1, 20 parameter
    for (; i<20 && i<pcaFullShape->eigenvalues.rows; i++){
        curSigma2 -= pcaFullShape->eigenvalues.at<double>(i, 0);
    }
    sigma2Pyr[1] = curSigma2 / (nMarkPoints*2-4);
    pcaPyr[1].eigenvalues = pcaFullShape->eigenvalues.rowRange(0, i);
    pcaPyr[1].eigenvectors = pcaFullShape->eigenvectors.rowRange(0, i);
    pcaPyr[1].mean = pcaFullShape->mean;
    
    /*sigma2Pyr[2] = sigma2Pyr[1] = */sigma2Pyr[0] = sigma2;
    /*pcaPyr[2] = pcaPyr[1]= */pcaPyr[0] = *pcaShape;
}

void ASMModel::saveToFile(ModelFile& file)
{
    ShapeModel::saveToFile(file);
    
    file.writeInt(localFeatureRad);
    file.writeInt(ns);
    
    int i,j;
    int rows, cols;
    file.writeInt(rows = iCovarG[0][0].rows);
    file.writeInt(cols = iCovarG[0][0].cols);
    for (i=0;i<=pyramidLevel;i++){
        for (j=0;j<nMarkPoints;j++){
            for (int ii=0;ii<rows;ii++)
                for (int jj=0;jj<cols;jj++)
                    file.writeReal(iCovarG[i][j](ii, jj));
        }
    }
    
    file.writeInt(rows = meanG[0][0].rows);
    file.writeInt(cols = meanG[0][0].cols);
    for (i=0;i<=pyramidLevel;i++){
        for (j=0;j<nMarkPoints;j++){
            for (int ii=0;ii<rows;ii++)
                for (int jj=0;jj<cols;jj++)
                    file.writeReal(meanG[i][j](ii, jj));
        }
    }
}
