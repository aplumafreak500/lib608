#include "windows.h"
#include "avisynth.h"
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <map>
#include <string>
#include <fstream>
#include <cmath>
using namespace std;

class DumpPixelValues : public GenericVideoFilter 
{
public:
	DumpPixelValues(PClip _child, const char* colorArg, const char* pointArg, const char* filenameArg, IScriptEnvironment* env);
	void WriteBin(IScriptEnvironment* env);
	PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
private:
	char* colorsWanted;
	/* Possible Values:
	   'A' = Alpha (either colorspace)
	   'R' = Red (RGB colorspace)
	   'G' = Green (RGB colorspace)
	   'B' = Blue (RGB colorspace)
	   'Y' = Luminance (YUV colorspace)
	   'U' = Chroma-blue (YUV colorspace)
	   'V' = Chroma-red (YUV colorspace)
    */
	struct point 
	{ 
		int x, y; 
		char* errorMsg;
		bool error;
		point::point(void) { x=0; y=0; }
		point::point(int xArg, int yArg)
		{
			x = xArg;
			y = yArg;
			error = false;
		}
		point::point(const char* pointArg)
		{
			const string pointString = string(pointArg);
			if (pointString.find('(') != 0)
			{
				sprintf(errorMsg, "DumpPixelValues: point %s not in format (x,y)", pointArg);
				error = true;
				return;
			}
			if (pointString.find(')') != (pointString.size() - 1))
			{
				sprintf(errorMsg, "DumpPixelValues: point '%s' not in format '(x,y)'", pointArg);
				error = true;
				return;
			}
			if ((pointString.find(',') == string.npos) || (pointString.find(',') != pointString.find_last_of(',')))
			{
				sprintf(errorMsg, "DumpPixelValues: point '%s' not in format '(x,y)'", pointArg);
				error = true;
				return;
			}
			const string xString = pointString.substr(1, pointString.find(',')-1);
			const string yString = pointString.substr(pointString.find(',')+1);
			x = atoi(xString.c_str());
			y = atoi(yString.c_str());
			error = false;
			return;
		}
		bool less (const point& first, const point& second)
		{
			if (first.y == second.y)
				return (first.x < second.x);
			return (first.y < second.y);
		}
		point operator= (const point* assignPoint)
		{
			point returnPoint(assignPoint->x, assignPoint->y);
			return returnPoint;
		}
		friend bool operator< (const point& first, const point& second)
		{
			if (first.y == second.y)
				return (first.x < second.x);
			return (first.y < second.y);
		}
	};
	vector<point> coordinates;
	const char* coordinatesString;
	struct pixelColor 
	{
		int A, R, G, B, Y, U, V;
		pixelColor::pixelColor(void)
		{
			A=-1; R=-1; G=-1; B=-1; Y=-1; U=-1; V=-1;
		}
		void reset(void)
		{
			A=-1; R=-1; G=-1; B=-1; Y=-1; U=-1; V=-1;
		}
	};
	//typedef map< point, pixelColor, less<point> > colors_type;
	//colors_type colors;
	//typedef colors_type::value_type value_type;
	map< point, pixelColor, less<point> > colors;
	char* filename;
};
