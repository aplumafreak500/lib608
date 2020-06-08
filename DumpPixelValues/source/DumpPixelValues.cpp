/*
DumpPixelValues plugin for Avisynth -- output color data to text or binary file
Copyright (C) 2004 McPoodle, All Rights Reserved
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation.
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.
You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
The author can be contacted at:
McPoodle
mcpoodle43@yahoo.com

V1.0 - 1st release.
*/

#include "DumpPixelValues.h"

// CDumpPixelValues

// CDumpPixelValuesApp construction

DumpPixelValues::DumpPixelValues(PClip _child, const char* colorArg, const char* pointArg, const char* filenameArg, IScriptEnvironment* env) : GenericVideoFilter(_child)
{
	// basic error checking
	if (vi.IsPlanar()) // is input planar? 
		env->ThrowError("DumpPixelValues: input to filter must be in YUY2 or RGB");   

	const string RGBcolors = "RGBA";
	const string YUVcolors = "YUV";
	const string colorInput = string(colorArg);
	if (colorInput.find_first_not_of(RGBcolors + YUVcolors) != string.npos)
		env->ThrowError("DumpPixelValues: color string can only be one or more of\nR, G, B, A, Y, U, V");
	if (RGBcolors.find_first_of(colorInput) && YUVcolors.find_first_of(colorInput))
		env->ThrowError("DumpPixelValues: colors must be in same colorspace (RGB or YUV)");
	if ((RGBcolors.find_first_of(colorInput) == string.npos) && (vi.IsRGB()))
		env->ThrowError("DumpPixelValues: colorspace must be YUV to get YUV colors");
	if ((YUVcolors.find_first_of(colorInput) == string.npos) && (vi.IsYUV()))
		env->ThrowError("DumpPixelValues: colorspace must be RGB to get RGB colors");
	colorsWanted = env->SaveString(colorInput.c_str());
	
	coordinatesString = pointArg;
	const string pointInput = string(pointArg);
	string::size_type pos1 = 0;
	string::size_type pos2 = 0;
	string::size_type posParens = 0;
	string pointString;
	point aPoint;
	while (pos2 != string.npos)
	{
		posParens = pointInput.find(')', pos1);
		if (posParens == string.npos)
			env->ThrowError("DumpPixelValues: wrong format for points input: '%s', %d, %d, %d", pointInput.c_str(), pos1, pos2, posParens);
		pos2 = pointInput.find(',', posParens);
		pointString = pointInput.substr(pos1,posParens - pos1 + 1);
		aPoint = point(pointString.c_str());
		if (aPoint.error)
			env->ThrowError(aPoint.errorMsg);
		coordinates.insert(coordinates.end(), aPoint);
		pos1 = pos2 + 1;
	}
	filename = env->SaveString(filenameArg);
}

void DumpPixelValues::WriteBin(IScriptEnvironment* env)
{
	fstream logFile;
	// write header if opening file for first time
	logFile.open(filename, ios::in);
	logFile.close();
	if (!logFile)
	{
		logFile.clear();
		logFile.open(filename, ios::out | ios::binary);
		// broadcast raw caption header
		logFile << "\xff\xff\xff\xff";
		logFile.close();
	}
	logFile.open(filename, ios::out | ios::app | ios::binary);
	vector<point>::iterator it;
	string colorsString = string(colorsWanted);
	int colorValue;
	int position = 7;
	int byte = 0;
	int test = 0;
	for (it = coordinates.begin(); it != coordinates.end(); it++)
	{
		colorValue = colors[*it].Y;
		byte += (int)pow(2, position) * (colorValue > 64);
		if (position == 8)
		{
			env->ThrowError("\nValue: %d", (int)pow(2, position) * (colorValue > 64));
		}
		position--;
		if (position < 0)
		{
			if (test == 2)
			{
				env->ThrowError("\nOutput: %d", byte);
			}
			logFile << (char)byte;
			position = 7;
			byte = 0;
			test = 1;
		}
	}
	logFile.close();
}

PVideoFrame __stdcall DumpPixelValues::GetFrame(int n, IScriptEnvironment* env)
{
	PVideoFrame src = child->GetFrame(n, env);
	PVideoFrame dst = env->NewVideoFrame(vi);
	const unsigned char* srcp = src->GetReadPtr();

	int x, y;
	point thisPoint;
	pixelColor pointColors;
	// Note that pointer starts at lower-left pixel for RGB
	unsigned char* dstp = dst->GetWritePtr();
	const int dst_pitch = dst->GetPitch();
	const int dst_width = dst->GetRowSize();
	const int dst_height = dst->GetHeight();
	const int src_pitch = src->GetPitch();
	const int src_width = src->GetRowSize();
	int unit_width_bytes = 1;
	int unit_width_pixels = 1;
	if (vi.IsRGB32())
	{
		unit_width_bytes = 4;
		unit_width_pixels = 4;
	}
	if (vi.IsYUY2())
	{
		unit_width_bytes = 4;
		unit_width_pixels = 2;
	}
	const int src_height = src->GetHeight();
	int w, h;
	char colorUnit;
	int colorR, colorG, colorB, colorA;
	int colorY1, colorU, colorY2, colorV;
	for (h=0; h < src_height; h++)
	{
		if (vi.IsRGB())
			y = src_height - h;
		else
			y = h;
		for (w=0; w < src_width; w += unit_width_bytes) 
		{
			x = w / unit_width_pixels;
			pointColors.reset();
			if (vi.IsRGB32())
			{
				colorUnit = *(srcp + w + 0);
				*(dstp + w + 0) = colorUnit; 
				colorB = (int)colorUnit;
				colorUnit = *(srcp + w + 1);
				*(dstp + w + 1) = colorUnit; 
				colorG = (int)colorUnit;
				colorUnit = *(srcp + w + 2);
				*(dstp + w + 2) = colorUnit; 
				colorR = (int)colorUnit;
				colorUnit = *(srcp + w + 3);
				*(dstp + w + 3) = colorUnit; 
				colorA = (int)colorUnit;
				if (colorR < 0) { colorR += 255; }
				if (colorG < 0) { colorG += 255; }
				if (colorB < 0) { colorB += 255; }
				if (colorA < 0) { colorA += 255; }

				pointColors.R = colorR;
				pointColors.G = colorG;
				pointColors.B = colorB;
				pointColors.A = colorA;
				thisPoint = point(x, y);
				colors[thisPoint] = pointColors;
			}
			if (vi.IsYUY2())
			{
				colorUnit = *(srcp + w + 0);
				*(dstp + w + 0) = colorUnit; 
				colorY1 = (int)colorUnit;
				colorUnit = *(srcp + w + 1);
				*(dstp + w + 1) = colorUnit; 
				colorU = (int)colorUnit;
				colorUnit = *(srcp + w + 2);
				*(dstp + w + 2) = colorUnit; 
				colorY2 = (int)colorUnit;
				colorUnit = *(srcp + w + 3);
				*(dstp + w + 3) = colorUnit; 
				colorV = (int)colorUnit;
				if (colorY1 < 0) { colorY1 += 255; }
				if (colorU < 0) { colorU += 255; }
				if (colorY2 < 0) { colorY2 += 255; }
				if (colorV < 0) { colorV += 255; }

				pointColors.Y = colorY1;
				pointColors.U = colorU;
				pointColors.V = colorV;
				thisPoint = point(x, y);
				colors[thisPoint] = pointColors;
				pointColors.Y = colorY2;
				thisPoint = point(x+1, y);
				colors[thisPoint] = pointColors;
			}
		}
		srcp = srcp + src_pitch;
		dstp = dstp + dst_pitch;
	}
	string filenameString = string(filename);
	if (filenameString.find(".bin") != string.npos)
	{
		WriteBin(env);
	}
	else
	{
		fstream logFile;
		// write header if opening file for first time
		logFile.open(filename, ios::in);
		logFile.close();
		if (!logFile)
		{
			logFile.clear();
			logFile.open(filename, ios::out);
			logFile << "DumpPixelValues for " << colorsWanted << ": " << coordinatesString << endl;
			logFile.close();
		}
		logFile.open(filename, ios::out | ios::app);
		vector<point>::iterator it;
		string colorsString = string(colorsWanted);
		int colorValue;
		char colorValueString[2];
		for (it = coordinates.begin(); it != coordinates.end(); it++)
		{
			for (unsigned int j=0; j < colorsString.size(); j++)
			{
				if (j>0) { logFile << "/"; }
				switch (colorsString[j])
				{
				case ('R'):
					colorValue = colors[*it].R;
					break;
				case ('G'):
					colorValue = colors[*it].G;
					break;
				case ('B'):
					colorValue = colors[*it].B;
					break;
				case ('A'):
					colorValue = colors[*it].A;
					break;
				case ('Y'):
					colorValue = colors[*it].Y;
					break;
				case ('U'):
					colorValue = colors[*it].U;
					break;
				case ('V'):
					colorValue = colors[*it].V;
					break;
				}
				sprintf(colorValueString, "%02X", colorValue);
				logFile << colorValueString;
			}
			logFile << " ";
		}
		logFile << endl;
		logFile.close();
		//point test = point(600,300);
		//env->ThrowError("Last Y: %d", colors[test].Y);
	}

	return dst;
}

AVSValue __cdecl Create_DumpPixelValues(AVSValue args, void* user_data, IScriptEnvironment* env) 
{
	// args[0] is the clip
	// args[1] is a string containing the colors wanted
	// args[2] is a string list of points (x,y) to collect 
	//  colors for; example "(50,100),(150,200)"
	// args[3] (optional) is the (escaped) path and name of 
	//  the file to write to
	const char* filenameArg;
	if (args.ArraySize() == 3)
		filenameArg = "C:\\DumpPixelValues.log";
	else
		filenameArg = args[3].AsString();
	return new DumpPixelValues(args[0].AsClip(), args[1].AsString(), args[2].AsString(), filenameArg, env);
}

extern "C" __declspec(dllexport) const char* __stdcall AvisynthPluginInit2(IScriptEnvironment* env) 
{
  env->AddFunction("DumpPixelValues", "css[filename]s", Create_DumpPixelValues, 0);
  return "`DumpPixelValues' DumpPixelValues plugin";
}
