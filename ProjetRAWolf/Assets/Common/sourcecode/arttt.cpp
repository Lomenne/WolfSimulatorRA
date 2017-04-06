#include <opencv/cv.h>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/calib3d/calib3d.hpp>
#include <opencv2/video/video.hpp>
#include <sys/timeb.h>

//#include "../aruco-1.3.0/src/aruco.h"
//#include "../aruco-1.3.0/src/cvdrawingutils.h"
#include <aruco/aruco.h>
#include <aruco/cvdrawingutils.h>

#include <string>

int CLOCK() {
	struct timespec t;
	clock_gettime(CLOCK_MONOTONIC, &t);
	return (t.tv_sec * 1000) + (t.tv_nsec * 1e-6);
}

cv::Scalar VISUAL_OBJECT_X_COLOR(128, 0, 0);
cv::Scalar VISUAL_OBJECT_O_COLOR(0, 128, 128);
cv::Scalar VISUAL_OBJECT_LAST_COLOR(0, 128, 0);
cv::Scalar VISUAL_OBJECT_INVALID_COLOR(0, 0, 255);

int XO_Board_State[9]; /* 0 to 8 */
int XO_Last_Move = 0;
int XO_Invalid_Move = -1;
int XO_Turn=0;
int XO_Player_Turn=1;
int XO_Total_Games=0;
int XO_Win_Games=0;
int XO_Lost_Games=0;
int XO_Drawing_Objects_Init = 0;
int XO_Winning_Line[3];


/* AI CODE IS FROM:

//Tic-tac-toe playing AI. Exhaustive tree-search. WTFPL
//Matthew Steel 2009, www.www.repsilat.com

*/
int win2(const int board[9]) {
    //determines if a player has won, returns 0 otherwise.
    unsigned wins[8][3] = {{0,1,2},{3,4,5},{6,7,8},{0,3,6},{1,4,7},{2,5,8},{0,4,8},{2,4,6}};
    int i;
    for(i = 0; i < 8; i++) {
        if(board[wins[i][0]] != 0 && 
           board[wins[i][0]] == board[wins[i][1]] &&	
           board[wins[i][0]] == board[wins[i][2]]) {
		XO_Winning_Line[0]=wins[i][0];
		XO_Winning_Line[1]=wins[i][1];
		XO_Winning_Line[2]=wins[i][2];
            	return board[wins[i][2]];
   	   }
    }
    return 0;
}
int win(const int board[9]) {
    //determines if a player has won, returns 0 otherwise.
    unsigned wins[8][3] = {{0,1,2},{3,4,5},{6,7,8},{0,3,6},{1,4,7},{2,5,8},{0,4,8},{2,4,6}};
    int i;
    for(i = 0; i < 8; i++) {
        if(board[wins[i][0]] != 0 && 
           board[wins[i][0]] == board[wins[i][1]] &&	
           board[wins[i][0]] == board[wins[i][2]])
            	return board[wins[i][2]];
    }
    return 0;
}
int minimax(int board[9], int player) {
    //How is the position like for player (their turn) on board?
    int winner = win(board);
    if(winner != 0) return winner*player;

    int move = -1;
    int score = -2;//Losing moves are preferred to no move
    int i;
    for(i = 0; i < 9; ++i) {//For all moves,
        if(board[i] == 0) {//If legal,
            board[i] = player;//Try the move
            int thisScore = -minimax(board, player*-1);
            if(thisScore > score) {
                score = thisScore;
                move = i;
            }//Pick the one that's worst for the opponent
            board[i] = 0;//Reset board after try
        }
    }
    if(move == -1) return 0;
    return score;
}
void checkEndGame(int board[9]) {
	int state=win2(board);

	if (XO_Turn==9) XO_Total_Games++;

	if (state!=0) {
		if (XO_Turn!=9) XO_Total_Games++;
		XO_Turn=9;
		if (state==1) 
			XO_Lost_Games++;
		else
			XO_Win_Games++;
	}
}
void computerMove(int board[9]) {
    int move = -1;
    int score = -2;
    int i;
    for(i = 0; i < 9; ++i) {
        if(board[i] == 0) {
            board[i] = 1;
            int tempScore = -minimax(board, -1);
            board[i] = 0;
            if(tempScore > score) {
                score = tempScore;
                move = i;
            }
        }
    }
    //returns a score based on minimax tree at a given node.
    board[move] = 1;
    XO_Last_Move=move;
    XO_Turn++;
	
	// check win :
	checkEndGame(board);
}
void playerMove(int board[9], int move) {
	if (XO_Turn>8) return;

	XO_Invalid_Move=-1;
	if (move < 9 && move >= 0 && board[move] == 0) {
	    	board[move] = -1;
    		XO_Last_Move=move;
		XO_Turn++;
		checkEndGame(board);
	} else
		XO_Invalid_Move=move;
}





cv::Mat objectPoints(397, 3, CV_32FC1);
void drawXOboard(cv::Mat &image, aruco::Board &B, const aruco::CameraParameters &CP) {
	float size = B[0].ssize;
	float hsize = size / 2.0;
	float size_x = 3.5*size;
	float size_y = 2.8*size;

if (XO_Drawing_Objects_Init == 0) {
	XO_Drawing_Objects_Init = 1;
	/* first four points are board corners bottom-left to top-left */
	objectPoints.at<float>(0,0) = -size_x;
	objectPoints.at<float>(0,1) = -size_y;
	objectPoints.at<float>(0,2) = 0;
	objectPoints.at<float>(1,0) = size_x;
	objectPoints.at<float>(1,1) = -size_y;
	objectPoints.at<float>(1,2) = 0;
	objectPoints.at<float>(2,0) = size_x;
	objectPoints.at<float>(2,1) = size_y;
	objectPoints.at<float>(2,2) = 0;
	objectPoints.at<float>(3,0) = -size_x;
	objectPoints.at<float>(3,1) = size_y;
	objectPoints.at<float>(3,2) = 0;

	/* next 8 points are XO table lines */
	float size15 = hsize * 3.0;
	float shadow_z = hsize;
	float face_z = hsize / 1.5;
	objectPoints.at<float>(4,0) = -hsize;
	objectPoints.at<float>(4,1) = -size15 ;
	objectPoints.at<float>(4,2) = shadow_z;
	objectPoints.at<float>(5,0) = -hsize;
	objectPoints.at<float>(5,1) = size15 ;
	objectPoints.at<float>(5,2) = shadow_z;
	objectPoints.at<float>(6,0) = hsize;
	objectPoints.at<float>(6,1) = -size15;
	objectPoints.at<float>(6,2) = shadow_z;
	objectPoints.at<float>(7,0) = hsize;
	objectPoints.at<float>(7,1) = size15;
	objectPoints.at<float>(7,2) = shadow_z;
	objectPoints.at<float>(8,0) = -size15;
	objectPoints.at<float>(8,1) = -hsize;
	objectPoints.at<float>(8,2) = shadow_z;
	objectPoints.at<float>(9,0) = size15 ;
	objectPoints.at<float>(9,1) = -hsize;
	objectPoints.at<float>(9,2) = shadow_z;
	objectPoints.at<float>(10,0) = -size15 ;
	objectPoints.at<float>(10,1) = hsize;
	objectPoints.at<float>(10,2) = shadow_z;
	objectPoints.at<float>(11,0) = size15 ;
	objectPoints.at<float>(11,1) = hsize;
	objectPoints.at<float>(11,2) = shadow_z;

	objectPoints.at<float>(12,0) = -hsize;
	objectPoints.at<float>(12,1) = -size15 ;
	objectPoints.at<float>(12,2) = face_z;
	objectPoints.at<float>(13,0) = -hsize;
	objectPoints.at<float>(13,1) = size15 ;
	objectPoints.at<float>(13,2) = face_z;
	objectPoints.at<float>(14,0) = hsize;
	objectPoints.at<float>(14,1) = -size15;
	objectPoints.at<float>(14,2) = face_z;
	objectPoints.at<float>(15,0) = hsize;
	objectPoints.at<float>(15,1) = size15;
	objectPoints.at<float>(15,2) = face_z;
	objectPoints.at<float>(16,0) = -size15;
	objectPoints.at<float>(16,1) = -hsize;
	objectPoints.at<float>(16,2) = face_z;
	objectPoints.at<float>(17,0) = size15 ;
	objectPoints.at<float>(17,1) = -hsize;
	objectPoints.at<float>(17,2) = face_z;
	objectPoints.at<float>(18,0) = -size15 ;
	objectPoints.at<float>(18,1) = hsize;
	objectPoints.at<float>(18,2) = face_z;
	objectPoints.at<float>(19,0) = size15 ;
	objectPoints.at<float>(19,1) = hsize;
	objectPoints.at<float>(19,2) = face_z;

	/* next 8 points are XO table center rectangle */
	objectPoints.at<float>(20,0) = -hsize;
	objectPoints.at<float>(20,1) = -hsize;
	objectPoints.at<float>(20,2) = shadow_z;
	objectPoints.at<float>(21,0) = hsize ;
	objectPoints.at<float>(21,1) = -hsize;
	objectPoints.at<float>(21,2) = shadow_z;
	objectPoints.at<float>(22,0) = hsize ;
	objectPoints.at<float>(22,1) = hsize;
	objectPoints.at<float>(22,2) = shadow_z;
	objectPoints.at<float>(23,0) = -hsize ;
	objectPoints.at<float>(23,1) = hsize;
	objectPoints.at<float>(23,2) = shadow_z;

	objectPoints.at<float>(24,0) = -hsize;
	objectPoints.at<float>(24,1) = -hsize;
	objectPoints.at<float>(24,2) = face_z;
	objectPoints.at<float>(25,0) = hsize ;
	objectPoints.at<float>(25,1) = -hsize;
	objectPoints.at<float>(25,2) = face_z;
	objectPoints.at<float>(26,0) = hsize ;
	objectPoints.at<float>(26,1) = hsize;
	objectPoints.at<float>(26,2) = face_z;
	objectPoints.at<float>(27,0) = -hsize ;
	objectPoints.at<float>(27,1) = hsize;
	objectPoints.at<float>(27,2) = face_z;

	/* points corresponding to center of each of the XO cells */
	objectPoints.at<float>(28,0) = size;
	objectPoints.at<float>(28,1) = size;
	objectPoints.at<float>(28,2) = shadow_z;
	objectPoints.at<float>(29,0) = 0 ;
	objectPoints.at<float>(29,1) = size;
	objectPoints.at<float>(29,2) = shadow_z;
	objectPoints.at<float>(30,0) = -size;
	objectPoints.at<float>(30,1) = size;
	objectPoints.at<float>(30,2) = shadow_z;
	objectPoints.at<float>(31,0) = size;
	objectPoints.at<float>(31,1) = 0;
	objectPoints.at<float>(31,2) = shadow_z;
	objectPoints.at<float>(32,0) = 0;
	objectPoints.at<float>(32,1) = 0;
	objectPoints.at<float>(32,2) = shadow_z;
	objectPoints.at<float>(33,0) = -size;
	objectPoints.at<float>(33,1) = 0;
	objectPoints.at<float>(33,2) = shadow_z;
	objectPoints.at<float>(34,0) = size;
	objectPoints.at<float>(34,1) = -size;
	objectPoints.at<float>(34,2) = shadow_z;
	objectPoints.at<float>(35,0) = 0;
	objectPoints.at<float>(35,1) = -size;
	objectPoints.at<float>(35,2) = shadow_z;
	objectPoints.at<float>(36,0) = -size;
	objectPoints.at<float>(36,1) = -size;
	objectPoints.at<float>(36,2) = shadow_z;

	/* We will have X at every cell, then O at every cell */
	float size8 = size / 8.0;
	float size4 = size / 4.0;
	float size816 = size8 * 1.5;
	objectPoints.at<float>(37,0) = size4;
	objectPoints.at<float>(37,1) = size4;
	objectPoints.at<float>(37,2) = shadow_z;
	objectPoints.at<float>(38,0) = size8;
	objectPoints.at<float>(38,1) = size4;
	objectPoints.at<float>(38,2) = shadow_z;
	objectPoints.at<float>(39,0) = 0;
	objectPoints.at<float>(39,1) = size8;
	objectPoints.at<float>(39,2) = shadow_z;
	objectPoints.at<float>(40,0) = -size8;
	objectPoints.at<float>(40,1) = size4;
	objectPoints.at<float>(40,2) = shadow_z;
	objectPoints.at<float>(41,0) = -size4;
	objectPoints.at<float>(41,1) = size4;
	objectPoints.at<float>(41,2) = shadow_z;
	objectPoints.at<float>(42,0) = -size8;
	objectPoints.at<float>(42,1) = 0;
	objectPoints.at<float>(42,2) = shadow_z;
	objectPoints.at<float>(43,0) = -size4;
	objectPoints.at<float>(43,1) = -size4;
	objectPoints.at<float>(43,2) = shadow_z;
	objectPoints.at<float>(44,0) = -size8;
	objectPoints.at<float>(44,1) = -size4;
	objectPoints.at<float>(44,2) = shadow_z;
	objectPoints.at<float>(45,0) = 0;
	objectPoints.at<float>(45,1) = -size8;
	objectPoints.at<float>(45,2) = shadow_z;
	objectPoints.at<float>(46,0) = size8;
	objectPoints.at<float>(46,1) = -size4;
	objectPoints.at<float>(46,2) = shadow_z;
	objectPoints.at<float>(47,0) = size4;
	objectPoints.at<float>(47,1) = -size4;
	objectPoints.at<float>(47,2) = shadow_z;
	objectPoints.at<float>(48,0) = size8;
	objectPoints.at<float>(48,1) = 0;
	objectPoints.at<float>(48,2) = shadow_z;

	for (int j=1; j<9; j++) {
		for (int i=0; i<12; i++) {
			objectPoints.at<float>(37+i+j*12,0)=objectPoints.at<float>(37+i, 0);
			objectPoints.at<float>(37+i+j*12,1)=objectPoints.at<float>(37+i, 1);
		}
	}	

	for (int j=0; j<9; j++) {
		for (int i=0; i<12; i++) {
			objectPoints.at<float>(37+i+j*12, 0)+=objectPoints.at<float>(28+j, 0);
			objectPoints.at<float>(37+i+j*12, 1)+=objectPoints.at<float>(28+j,1);
			objectPoints.at<float>(37+i+j*12, 2)=objectPoints.at<float>(28+j,2);

			objectPoints.at<float>(145+i+j*12,0)=objectPoints.at<float>(37+i+j*12,0);
			objectPoints.at<float>(145+i+j*12,1)=objectPoints.at<float>(37+i+j*12,1);
			objectPoints.at<float>(145+i+j*12,2)=face_z;

		}
	}	

	objectPoints.at<float>(253,0) = 0;
	objectPoints.at<float>(253,1) = size4;
	objectPoints.at<float>(253,2) = shadow_z;
	objectPoints.at<float>(254,0) = -size816;
	objectPoints.at<float>(254,1) = size816;
	objectPoints.at<float>(254,2) = shadow_z;
	objectPoints.at<float>(255,0) = -size4;
	objectPoints.at<float>(255,1) = 0;
	objectPoints.at<float>(255,2) = shadow_z;
	objectPoints.at<float>(256,0) = -size816;
	objectPoints.at<float>(256,1) = -size816;
	objectPoints.at<float>(256,2) = shadow_z;
	objectPoints.at<float>(257,0) = 0;
	objectPoints.at<float>(257,1) = -size4;
	objectPoints.at<float>(257,2) = shadow_z;
	objectPoints.at<float>(258,0) = size816;
	objectPoints.at<float>(258,1) = -size816;
	objectPoints.at<float>(258,2) = shadow_z;
	objectPoints.at<float>(259,0) = size4;
	objectPoints.at<float>(259,1) = 0;
	objectPoints.at<float>(259,2) = shadow_z;
	objectPoints.at<float>(260,0) = size816;
	objectPoints.at<float>(260,1) = size816;
	objectPoints.at<float>(260,2) = shadow_z;
	
	for (int j=1; j<9; j++) {
		for (int i=0; i<8; i++) {
			objectPoints.at<float>(253+i+j*8,0)=objectPoints.at<float>(253+i, 0);
			objectPoints.at<float>(253+i+j*8,1)=objectPoints.at<float>(253+i, 1);
		}
	}	

	for (int j=0; j<9; j++) {
		for (int i=0; i<8; i++) {
			objectPoints.at<float>(253+i+j*8, 0)+=objectPoints.at<float>(28+j, 0);
			objectPoints.at<float>(253+i+j*8, 1)+=objectPoints.at<float>(28+j,1);
			objectPoints.at<float>(253+i+j*8, 2)=objectPoints.at<float>(28+j,2);

			objectPoints.at<float>(325+i+j*8,0)=objectPoints.at<float>(253+i+j*8,0);
			objectPoints.at<float>(325+i+j*8,1)=objectPoints.at<float>(253+i+j*8,1);
			objectPoints.at<float>(325+i+j*8,2)=face_z;

		}
	}	
}

	std::vector<cv::Point2f> imagePoints;
	cv::projectPoints(objectPoints, B.Rvec, B.Tvec, CP.CameraMatrix, CP.Distorsion, imagePoints);

	/* draw polygon */
	cv::Point vertices[4];
	for (int i=0; i<4; i++) {
		vertices[i]=imagePoints[i];
	}
	cv::fillConvexPoly(image, vertices, 4, cv::Scalar(255, 255, 255), 8, 0);


//	for (int i=4; i<12; i+=2)  
//		cv::line(image, imagePoints[i], imagePoints[i+1], cv::Scalar(240, 240, 240), 2, CV_AA);
	/* draw 3D back */
	for (int i=0; i<4; i++) {
		int index = i * 2;
		vertices[0]=imagePoints[index+4];
		vertices[1]=imagePoints[index+12];
		vertices[2]=imagePoints[index+13];
		vertices[3]=imagePoints[index+5];
		cv::fillConvexPoly(image, vertices, 4, cv::Scalar(240, 240, 240), 8, 0);
		cv::line(image, vertices[0], vertices[1], cv::Scalar(200, 200, 200), 1, CV_AA);
		cv::line(image, vertices[2], vertices[3], cv::Scalar(200, 200, 200), 1, CV_AA);
		cv::line(image, vertices[3], vertices[0], cv::Scalar(200, 200, 200), 1, CV_AA);

	}

	for (int i=0; i<4; i++)
		cv::line(image, imagePoints[20+i], imagePoints[24+i], cv::Scalar(200, 200, 200), 1, CV_AA);

	for (int i=12; i<20; i+=2) 
		cv::line(image, imagePoints[i], imagePoints[i+1], cv::Scalar(32, 23, 123), 2, CV_AA);

	/* render board state */
//	cv::Point2f offsetPoint = imagePoints[12] - imagePoints[4];
//	offsetPoint.y *= 30.0;

	for (int i=0; i<9; i++) {
		std::string label;
		if (XO_Board_State[i]!=0) {
//			if (XO_Board_State[i]==1)
//				label = "O";
//			else
//				label = "X";

			if (XO_Board_State[i]==1) {
				cv::Scalar face_color = VISUAL_OBJECT_O_COLOR;
				if (XO_Invalid_Move==i || XO_Winning_Line[0]==i || XO_Winning_Line[1]==i || XO_Winning_Line[2] ==i) face_color = VISUAL_OBJECT_INVALID_COLOR;
					else if (XO_Last_Move == i) face_color = VISUAL_OBJECT_LAST_COLOR;

				cv::Point vertices_O[8];
				for (int j=0; j<8; j++) {
					vertices_O[j]=imagePoints[325+j+i*8];
 				}
				for (int j=0; j<8; j++) {
					if (j<7)
						cv::line(image, imagePoints[253+j+i*8], imagePoints[254+j+i*8], cv::Scalar(200, 200, 200), 1, CV_AA);
					else
						cv::line(image, imagePoints[253+j+i*8], imagePoints[254+i*8], cv::Scalar(200, 200, 200), 1, CV_AA);

					cv::line(image, imagePoints[253+j+i*8], imagePoints[325+j+i*8], cv::Scalar(200, 200, 200), 1, CV_AA);

				}

				cv::fillConvexPoly(image, vertices_O, 8, face_color, 8, 0);


			} else {
				cv::Scalar face_color = VISUAL_OBJECT_X_COLOR;
				if (XO_Invalid_Move==i || XO_Winning_Line[0]==i || XO_Winning_Line[1]==i || XO_Winning_Line[2] ==i) face_color = VISUAL_OBJECT_INVALID_COLOR;
					else if (XO_Last_Move == i) face_color = VISUAL_OBJECT_LAST_COLOR;

//				cv::Point vertices_XB[1][12];
				cv::Point vertices_XF[1][12];
				for (int j=0; j<12; j++) {
//					vertices_XB[0][j]=imagePoints[37+j+i*12];
					vertices_XF[0][j]=imagePoints[145+j+i*12];
 				}
//				const cv::Point *ppt1[1] = {vertices_XB[0]};
				const cv::Point *ppt2[1] = {vertices_XF[0]};
				int npt[] = { 12 };

//				cv::fillPoly(image, ppt1, npt, 1, cv::Scalar(200,200,200), 8);

//				float inc_x = (vertices_XB[0][0].x-vertices_XF[0][0].x)/10.0;
//				float inc_y = (vertices_XB[0][0].y-vertices_XF[0][0].y)/10.0;
//				for (int j=0; j<10; j++) {
//					for (int k=0; k<12; k++) {
//						vertices_XB[0][k].x -= inc_x;
//						vertices_XB[0][k].y += inc_y;
//					}
//					cv::polylines(image, ppt1, npt, 1, 1, cv::Scalar(128,128,128), 1, 8);
//				}
				for (int j=0; j<12; j++) {
					if (j<11)
						cv::line(image, imagePoints[37+j+i*12], imagePoints[38+j+i*12], cv::Scalar(200, 200, 200), 1, CV_AA);
					else
						cv::line(image, imagePoints[37+j+i*12], imagePoints[37+i*12], cv::Scalar(200, 200, 200), 1, CV_AA);

					cv::line(image, imagePoints[37+j+i*12], imagePoints[145+j+i*12], cv::Scalar(200, 200, 200), 1, CV_AA);

				}

				cv::fillPoly(image, ppt2, npt, 1, face_color, 8);


			}

/*			for (int j=10; j>5; j--) {
				if (i==XO_Last_Move)
					cv::putText(image, label, imagePoints[28+i]-offsetPoint*j/10.0, CV_FONT_HERSHEY_SIMPLEX, 3, cv::Scalar(0, 255, 0), 2, 8);
				else
					cv::putText(image, label, imagePoints[28+i]-offsetPoint*j/10.0, CV_FONT_HERSHEY_SIMPLEX, 3, cv::Scalar(0, 0, 0), 2, 8);
			}*/

		}
	
	}
	
	cv::Point2f status;
	status.x=50;
	status.y=700;

	if (XO_Turn>8) {
		switch(win(XO_Board_State)) {
	        	case 0:
			cv::putText(image, "A Draw! Press SPC, R, or ESC.", status, CV_FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(255, 0, 0), 2, 8);
		            break;
		        case 1:
			cv::putText(image, "You Lost! Press SPC, R, or ESC.", status, CV_FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 0, 255), 2, 8);
	        	    break;
		        case -1:
			cv::putText(image, "You Won! Press SPC, R, or ESC.", status, CV_FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 255, 0), 2, 8);
	        	    break;
	    	}
	} else {
		if (XO_Invalid_Move!=-1) {
//			cv::putText(image, "X", imagePoints[28+XO_Invalid_Move]-offsetPoint, CV_FONT_HERSHEY_SIMPLEX, 3, cv::Scalar(0, 0, 255), 2, 8);
			cv::putText(image, "Invalid Move! Try again!", status, CV_FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 0, 255), 2, 8);
		} else if ((XO_Player_Turn + XO_Turn)%2 == 1) 
			cv::putText(image, "Your Turn!", status, CV_FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 0, 128), 2, 8);
		
		
		
	}

	int XO_Draws = (XO_Total_Games - XO_Win_Games - XO_Lost_Games);
	std::string info = "Score: " + std::to_string(XO_Win_Games) + " of " + std::to_string(XO_Lost_Games) + " - Draws: " + std::to_string(XO_Draws);
	cv::putText(image, info, status+cv::Point2f(770, 0), CV_FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(32, 64, 64), 2, 8);
	

}

int main(int argc, char **argv) {

XO_Board_State[0] = 0;
XO_Board_State[1] = 0;
XO_Board_State[2] = 0;
XO_Board_State[3] = 0;
XO_Board_State[4] = 0;
XO_Board_State[5] = 0;
XO_Board_State[6] = 0;
XO_Board_State[7] = 0;
XO_Board_State[8] = 0;
XO_Player_Turn = 1; /* computer plays on EVEN turns: XO_Player_Turn + XO_Turn */
XO_Turn = 0;
XO_Winning_Line[0]=-1;
XO_Winning_Line[1]=-1;
XO_Winning_Line[2]=-1;

cv::namedWindow( "left_window", CV_WINDOW_NORMAL ); 
cv::setWindowProperty( "left_window", CV_WND_PROP_FULLSCREEN, CV_WINDOW_FULLSCREEN);
cv::moveWindow( "left_window", 0, 0);

cv::VideoCapture capture;

capture.open(0);

capture.set(CV_CAP_PROP_FRAME_WIDTH, 1280);
capture.set(CV_CAP_PROP_FRAME_HEIGHT, 900);
capture.set(CV_CAP_PROP_FPS, 30);

//capture.set(CV_CAP_PROP_AUTO_EXPOSURE, CV_CAP_PROP_DC1394_OFF);
//capture.set(CV_CAP_PROP_FOURCC, CV_FOURCC('H', '2', '6', '4'));
//capture.set(CV_CAP_PROP_EXPOSURE, -2);
//capture.set(CV_CAP_PROP_GAIN, 200);
//capture.set(CV_CAP_PROP_BRIGHTNESS, 128);
//capture.set(CV_CAP_PROP_CONTRAST, 35);
//capture.set(CV_CAP_PROP_SATURATION, 30);
//capture.set(CV_CAP_PROP_SHARPNESS, 30);
//capture.set(CV_CAP_PROP_FOCUS, 0);
//capture.set(CV_CAP_PROP_PAN, 0);
//capture.set(CV_CAP_PROP_TILT, 0);
//capture.set(CV_CAP_PROP_ZOOM, 1);


int key=0;
int frames=0;
cv::Mat camera_frame;
clock_t start, end;
double seconds, fps;

aruco::MarkerDetector MDetector;
vector<aruco::Marker> Markers;
aruco::CameraParameters CamParam;
CamParam.readFromXMLFile("camera.yml");
float MarkerSize = 0.05;

aruco::BoardConfiguration TheBoardConfig;
aruco::BoardDetector TheBoardDetector;

TheBoardConfig.readFromFile("board.yml");
int camshouldresize = 1;

int flag_live = 2;
int flag_capture = 0;

while (key!=27) {
	frames++;
	if (frames==1)
		start = CLOCK();
//	cv::Mat camera_frame;
	capture >> camera_frame;
	if (camera_frame.data) {
		if (camshouldresize) {
			CamParam.resize(camera_frame.size());
			camshouldresize=0;
		}
		MDetector.detect(camera_frame, Markers, CamParam, MarkerSize);
		aruco::Board DetectedBoard;
		int probDetect = TheBoardDetector.detect(Markers, TheBoardConfig, DetectedBoard, CamParam, MarkerSize);

		if (flag_live==2) {
			if (CamParam.isValid() && Markers.size()) {
				if(XO_Turn<9 && (XO_Turn+XO_Player_Turn) % 2 == 0)
			            computerMove(XO_Board_State);
			        else {
					drawXOboard(camera_frame, DetectedBoard, CamParam);
		        	}
			}
		} else if (flag_live==1) {
			for (unsigned int i=0; i<Markers.size(); i++) {
				Markers[i].draw(camera_frame, cv::Scalar(0, 0, 255), 2);
			}

			if (CamParam.isValid()) {
				for (unsigned int i=0; i<Markers.size(); i++) {
					aruco::CvDrawingUtils::draw3dCube(camera_frame, Markers[i], CamParam, false);
					aruco::CvDrawingUtils::draw3dAxis(camera_frame, Markers[i], CamParam);
				
				}
				//if (Markers.size()) aruco::CvDrawingUtils::draw3dAxis(camera_frame, Markers[0], CamParam);
				if (Markers.size()) aruco::CvDrawingUtils::draw3dAxis(camera_frame, DetectedBoard, CamParam);
	//			std::cout << DetectedBoard.Rvec << " " << DetectedBoard.Tvec << std::endl;
	//			std::cout << "Prob: " << probDetect << std::endl;
//				if (Markers.size()) std::cout << DetectedBoard.size() << std::endl;
			
			}
		}

		cv::imshow( "left_window", camera_frame);
		if (flag_capture) {
			clock_t tm = CLOCK();
			std::string filename = "cap_" + std::to_string(tm) + ".jpg";
			imwrite(filename, camera_frame);
		}

	}

	if (frames==30) {
		end = CLOCK();
		seconds = end - start;
		fps = 30000.0 / seconds;

		printf("FPS: %0.6f\n", fps);
		frames=0;
	}

	key = cv::waitKey(1);

	switch (key) {
		case 'l': 
		case 'L':
			flag_live++;
			if (flag_live>2) flag_live=0;
			break;
		case 'O':
		case 'o':
			flag_capture=!flag_capture;
			break;
		case '7':
			playerMove(XO_Board_State, 0);
			break;
		case '8':
			playerMove(XO_Board_State, 1);
			break;
		case '9':
			playerMove(XO_Board_State, 2);
			break;
		case '4':
			playerMove(XO_Board_State, 3);
			break;
		case '5':
			playerMove(XO_Board_State, 4);
			break;
		case '6':
			playerMove(XO_Board_State, 5);
			break;
		case '1':
			playerMove(XO_Board_State, 6);
			break;
		case '2':
			playerMove(XO_Board_State, 7);
			break;
		case '3':
			playerMove(XO_Board_State, 8);
			break;
		case 'R':
		case 'r':
			XO_Turn=0;
			XO_Player_Turn=1;
			XO_Total_Games=0;
			XO_Win_Games=0;
			XO_Lost_Games=0;
				
			XO_Board_State[0] = 0;
			XO_Board_State[1] = 0;
			XO_Board_State[2] = 0;
			XO_Board_State[3] = 0;
			XO_Board_State[4] = 0;
			XO_Board_State[5] = 0;
			XO_Board_State[6] = 0;
			XO_Board_State[7] = 0;
			XO_Board_State[8] = 0;			

			XO_Winning_Line[0]=-1;
			XO_Winning_Line[1]=-1;
			XO_Winning_Line[2]=-1;
			break;
		case ' ':
			XO_Turn=0;
			int last_win=win(XO_Board_State);
			if (last_win==1)
				XO_Player_Turn = 0;
			else if (last_win==-1)
				XO_Player_Turn = 1;
				
			XO_Board_State[0] = 0;
			XO_Board_State[1] = 0;
			XO_Board_State[2] = 0;
			XO_Board_State[3] = 0;
			XO_Board_State[4] = 0;
			XO_Board_State[5] = 0;
			XO_Board_State[6] = 0;
			XO_Board_State[7] = 0;
			XO_Board_State[8] = 0;			
			
			XO_Winning_Line[0]=-1;
			XO_Winning_Line[1]=-1;
			XO_Winning_Line[2]=-1;
			
	}
}

//capture.release();
//cv::destroyWindow("left_window");

return 0;

}
