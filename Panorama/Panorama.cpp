// Imagine++ project
// Project:  Panorama
// Author:   Pascal Monasse
// Date:     2013/10/08

// Marius Dufraisse TP1 

#include <Imagine/Graphics.h>
#include <Imagine/Images.h>
#include <Imagine/LinAlg.h>
#include <vector>
#include <sstream>
using namespace Imagine;
using namespace std;

void drawCross(int x, int y, Color color, int size = 5, int penWidth=2){
    drawLine(x-size, y-size, x+size, y+size, color, penWidth);
    drawLine(x-size, y+size, x+size, y-size, color, penWidth);
}


// Record clicks in two images, until right button click
void getClicks(Window w1, Window w2,
               vector<IntPoint2>& pts1, vector<IntPoint2>& pts2) {
    // Help window
    Window help = openWindow(450,100);
    setActiveWindow(help);
    drawString(0, 20,"Select at least 4 pairs of matching points in the two images.", BLACK);
    drawString(0, 40, "Select the last one using right to proceed to the next step.", BLACK);
    drawString(10,65, "Number of points :", BLACK);
    drawString(150,65, "0", BLACK);
    
    int x; int y; 
    int click = 0; // Last button clicked
    int i = 0; // Number of points stored
    while ((click != 3) || (i < 4)){
        setActiveWindow(w2);
        showWindow(w2);
        getMouse(x,y);
        drawCross(x, y, Color(255,0,0));
        pts2.push_back(IntPoint2(x,y));
        
        setActiveWindow(w1);
        showWindow(w1);
        click = getMouse(x,y);
        drawCross(x, y, Color(255,0,0));
        pts1.push_back(IntPoint2(x,y));

        i ++;
        // Write the current number of points
        setActiveWindow(help);
        fillRect(150,45,300,30,WHITE);
        drawString(150,65, to_string(i), BLACK);
    }

    closeWindow(help);
}

// Return homography compatible with point matches
Matrix<float> getHomography(const vector<IntPoint2>& pts1,
                            const vector<IntPoint2>& pts2) {
    size_t n = min(pts1.size(), pts2.size());
    if(n<4) {
        cout << "Not enough correspondences: " << n << endl;
        return Matrix<float>::Identity(3);
    }
    Matrix<double> A(2*n,8);
    Vector<double> B(2*n);

    IntPoint2 pt1; IntPoint2 pt2;
    for (int c = 0; c < n; c ++){
        // Retrieve two corresponding points (x1,y1) and (x2,y2)
        pt1 = pts1[c];
        int x1; int y1;
        x1 = pt1[0];
        y1 = pt1[1];
        
        pt2 = pts2[c];
        int x2; int y2;
        x2 = pt2[0];
        y2 = pt2[1];
       
        // Fill the matrices A and B
        B[2*c] = x2;
        B[2*c+1] = y2;

        A(2*c, 0) = x1; A(2*c, 1) = y1; A(2*c, 2) = 1; A(2*c, 3) = 0;
        A(2*c, 4) = 0; A(2*c, 5) = 0; A(2*c, 6) = -x1*x2; A(2*c, 7) = -x2*y1;

        A(2*c+1, 0) = 0; A(2*c+1, 1) = 0; A(2*c+1, 2) = 0; A(2*c+1, 3) = x1;
        A(2*c+1, 4) = y1; A(2*c+1, 5) = 1; A(2*c+1, 6) = -x1*y2; A(2*c+1, 7) = -y2*y1;
    }

    B = linSolve(A, B);
    Matrix<float> H(3, 3);
    H(0,0)=B[0]; H(0,1)=B[1]; H(0,2)=B[2];
    H(1,0)=B[3]; H(1,1)=B[4]; H(1,2)=B[5];
    H(2,0)=B[6]; H(2,1)=B[7]; H(2,2)=1;

    // Sanity check
    for(size_t i=0; i<n; i++) {
        float v1[]={(float)pts1[i].x(), (float)pts1[i].y(), 1.0f};
        float v2[]={(float)pts2[i].x(), (float)pts2[i].y(), 1.0f};
        Vector<float> x1(v1,3);
        Vector<float> x2(v2,3);
        x1 = H*x1;
        cout << x1[1]*x2[2]-x1[2]*x2[1] << ' '
             << x1[2]*x2[0]-x1[0]*x2[2] << ' '
             << x1[0]*x2[1]-x1[1]*x2[0] << endl;
    }
    return H;
}

// Grow rectangle of corners (x0,y0) and (x1,y1) to include (x,y)
void growTo(float& x0, float& y0, float& x1, float& y1, float x, float y) {
    if(x<x0) x0=x;
    if(x>x1) x1=x;
    if(y<y0) y0=y;
    if(y>y1) y1=y;    
}

// Panorama construction
void panorama(const Image<Color,2>& I1, const Image<Color,2>& I2,
              Matrix<float> H) {
    Vector<float> v(3);
    float x0=0, y0=0, x1=I2.width(), y1=I2.height();

    v[0]=0; v[1]=0; v[2]=1;
    v=H*v; v/=v[2];
    growTo(x0, y0, x1, y1, v[0], v[1]);

    v[0]=I1.width(); v[1]=0; v[2]=1;
    v=H*v; v/=v[2];
    growTo(x0, y0, x1, y1, v[0], v[1]);

    v[0]=I1.width(); v[1]=I1.height(); v[2]=1;
    v=H*v; v/=v[2];
    growTo(x0, y0, x1, y1, v[0], v[1]);

    v[0]=0; v[1]=I1.height(); v[2]=1;
    v=H*v; v/=v[2];
    growTo(x0, y0, x1, y1, v[0], v[1]);

    cout << "x0 x1 y0 y1=" << x0 << ' ' << x1 << ' ' << y0 << ' ' << y1<<endl;

    Matrix<float> Hinv = inverse(H);

    Image<Color> I(int(x1-x0), int(y1-y0));
    setActiveWindow( openWindow(I.width(), I.height()) );
    I.fill(WHITE);
    Color color1; Color color2; // Color of the corresponding pixel in images 1 and 2
    bool col1ok; bool col2ok; // true if the current pixel is in the image 1 or 2
    for (int x = int(x0); x < int(x1); x ++){
        for (int y = int(y0); y < int(y1); y++){
            col1ok = false;
            col2ok = false;

            // Color of the corresponding pixel in image 2
            if ((0 <= x && x < I2.width()) && (0 <= y && y < I2.height())){
                color2 = I2(x,y);
                col2ok = true;
            }

            // Color of the corresponding pixel in image 1
            v[0] = x; v[1] = y; v[2] = 1;
            v = Hinv*v; v/=v[2];
            int xp = int(v[0]);
            int yp = int(v[1]);
            if ((0 <= xp && xp < I1.width()) && (0 <= yp && yp < I1.height())) {
                color1 = I1(xp, yp);
                col1ok = true;
            }

            // Resulting color
            if (col1ok && col2ok) {
                I(x-x0,y-y0) = color1/byte(2) + color2/byte(2);
            }
            else if (col1ok){
                I(x-x0,y-y0) = color1;
            }
            else if (col2ok){
                I(x-x0,y-y0) = color2;
            }
        }
    }
    display(I,0,0);
}

// Main function
int main(int argc, char* argv[]) {
    const char* s1 = argc>1? argv[1]: srcPath("image0006.jpg");
    const char* s2 = argc>2? argv[2]: srcPath("image0007.jpg");

    // Load and display images
    Image<Color> I1, I2;
    if( ! load(I1, s1) ||
        ! load(I2, s2) ) {
        cerr<< "Unable to load the images" << endl;
        return 1;
    }
    Window w1 = openWindow(I1.width(), I1.height(), s1);
    display(I1,0,0);
    Window w2 = openWindow(I2.width(), I2.height(), s2);
    setActiveWindow(w2);
    display(I2,0,0);

    // Get user's clicks in images
    vector<IntPoint2> pts1, pts2;
    getClicks(w1, w2, pts1, pts2);

    vector<IntPoint2>::const_iterator it;
    cout << "pts1="<<endl;
    for(it=pts1.begin(); it != pts1.end(); it++)
        cout << *it << endl;
    cout << "pts2="<<endl;
    for(it=pts2.begin(); it != pts2.end(); it++)
        cout << *it << endl;

    // Compute homography
    Matrix<float> H = getHomography(pts1, pts2);
    cout << "H=" << H/H(2,2);

    // Apply homography
    panorama(I1, I2, H);

    endGraphics();
    return 0;
}
