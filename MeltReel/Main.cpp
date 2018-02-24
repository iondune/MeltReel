

#include <string>
#include <vector>
#include <fstream>
#include <sstream>

using namespace std;

#include "tinyformat.h"

ofstream outfile;


void Write(string s)
{
	std::replace(s.begin(), s.end(), '\'', '"');
	outfile << s;
}

void Line(string const & s)
{
	Write(s);
	outfile << endl;
}

void Line(int const indent, string const & s)
{
	for (int i = 0; i < indent; ++ i)
	{
		outfile << "  ";
	}
	Line(s);
}

void WriteHeader()
{
	Line("<?xml version='1.0'?>");
	Line("<mlt LC_NUMERIC='en_US.UTF-8' version='6.0.0' title='Reel'>");
	Line(1, "<profile description='automatic' width='1280' height='720' progressive='1' sample_aspect_num='1' sample_aspect_den='1' display_aspect_num='1280' display_aspect_den='720' frame_rate_num='25' frame_rate_den='1' colorspace='709'/>");
}

void WriteBlackClip()
{
	Line(1, "<producer id='black' in='0' out='24'>");
	Line(2, "<property name='mlt_type'>producer</property>");
	Line(2, "<property name='length'>15000</property>");
	Line(2, "<property name='eof'>pause</property>");
	Line(2, "<property name='resource'>black</property>");
	Line(2, "<property name='aspect_ratio'>1</property>");
	Line(2, "<property name='mlt_service'>colour</property>");
	Line(1, "</producer>");
}

void WriteSourceClip(int const id)
{
	Line(1, tfm::format("<producer id='source%d'>", id));
	Line(2, "<property name='mlt_type'>producer</property>");
	Line(2, tfm::format("<property name='resource'>../Raw/p%d.mkv</property>", id));
	Line(1, "</producer>");
}

void WriteTransitionOut(int const id, int const source, int const frame)
{
	Line(1, tfm::format("<tractor id='%dout' in='0' out='24'>", id));
	Line(2, tfm::format("<track producer='source%d' in='%d' out='%d'/>", source, frame, frame+24));
	Line(2, "<track producer='black' in='0' out='24'/>");
	Line(2, tfm::format("<transition id='%dout_luma' out='24'>", id));
	Line(3, "<property name='a_track'>0</property>");
	Line(3, "<property name='b_track'>1</property>");
	Line(3, "<property name='factory'>loader</property>");
	Line(3, "<property name='mlt_type'>transition</property>");
	Line(3, "<property name='mlt_service'>luma</property>");
	Line(2, "</transition>");
	Line(2, tfm::format("<transition id='%dout_mix' out='24'>", id));
	Line(3, "<property name='a_track'>0</property>");
	Line(3, "<property name='b_track'>1</property>");
	Line(3, "<property name='start'>-1</property>");
	Line(3, "<property name='mlt_type'>transition</property>");
	Line(3, "<property name='mlt_service'>mix</property>");
	Line(2, "</transition>");
	Line(1, "</tractor>");
}

void WriteTransitionIn(int const id, int const source, int const frame)
{
	Line(1, tfm::format("<tractor id='%din' in='0' out='24'>", id));
	Line(2, "<track producer='black' in='0' out='24'/>");
	Line(2, tfm::format("<track producer='source%d' in='%d' out='%d'/>", source, frame, frame+24));
	Line(2, tfm::format("<transition id='%din_luma' out='24'>", id));
	Line(3, "<property name='a_track'>0</property>");
	Line(3, "<property name='b_track'>1</property>");
	Line(3, "<property name='factory'>loader</property>");
	Line(3, "<property name='mlt_type'>transition</property>");
	Line(3, "<property name='mlt_service'>luma</property>");
	Line(2, "</transition>");
	Line(2, tfm::format("<transition id='%dout_mix' out='24'>", id));
	Line(3, "<property name='a_track'>0</property>");
	Line(3, "<property name='b_track'>1</property>");
	Line(3, "<property name='start'>-1</property>");
	Line(3, "<property name='mlt_type'>transition</property>");
	Line(3, "<property name='mlt_service'>mix</property>");
	Line(2, "</transition>");
	Line(1, "</tractor>");
}

const int frameRate = 25;

struct Clip
{
	int Source = -1;
	int Start = -1;
	int End = -1;
};

vector<string> Explode(string const & str, char delimiter)
{
	vector<string> Words;
	std::istringstream Stream(str);
	string Word;

	while (std::getline(Stream, Word, delimiter))
	{
		Words.push_back(move(Word));
	}

	return Words;
}

int HMStoFrames(string const & s)
{
	vector<string> elements = Explode(s, ':');

	if (elements.size() == 3)
	{
		const int H = stoi(elements[0]);
		const int M = stoi(elements[1]);
		const int S = stoi(elements[2]);

		return (H * 60 * 60 + M * 60 + S) * frameRate;
	}
	else if (elements.size() == 2)
	{
		const int M = stoi(elements[0]);
		const int S = stoi(elements[1]);

		return (M * 60 + S) * frameRate;
	}
	else
	{
		cerr << "Malformed time: " << s << endl;
		return -1;
	}
}

int main()
{
	outfile.open("reel.xml");

	Clip c1, c2;
	c1.Source = 3;
	c1.Start = HMStoFrames("1:28:59");
	c1.End = HMStoFrames("1:29:06");

	c2.Source = 4;
	c2.Start = HMStoFrames("1:28:59");
	c2.End = HMStoFrames("1:29:06");

	vector<Clip> clips;
	clips.push_back(c1);
	clips.push_back(c2);

	WriteHeader();
	WriteBlackClip();
	WriteSourceClip(3);
	WriteSourceClip(4);

	for (int i = 0; i < clips.size(); ++ i)
	{
		auto clip = clips[i];

		WriteTransitionIn(i, clip.Source, clip.Start);
		WriteTransitionOut(i, clip.Source, clip.End - 24);
	}

	//WriteTransitionOut(0, 3, 103);
	//WriteTransitionIn(1, 4, 0);


	Line(1, "<playlist id='playlist0'>");
	for (int i = 0; i < clips.size(); ++ i)
	{
		auto clip = clips[i];

		Line(2, tfm::format("<entry producer='%din' in='0' out='24'/>", i));
		Line(2, tfm::format("<entry producer='source%d' in='%d' out='%d'/>", clip.Source, clip.Start + 24 + 1, clip.End - 24 - 1));
		Line(2, tfm::format("<entry producer='%dout' in='0' out='24'/>", i));
	}
	Line(1, "</playlist>");

	Line(1, "<tractor id='tractor2' title='clip1.mkv' global_feed='1'>");
	Line(2, "<track producer='playlist0'/>");
	Line(1, "</tractor>");
	Line("</mlt>");

	return 0;
	/*
	ifstream input("clips.csv");
	char const field_delim = '\t';

	int last = 0;

	for (string row; getline(input, row); )
	{
		vector<string> values;
		istringstream ss(row);

		for (string field; getline(ss, field, field_delim); )
		{
			values.push_back(field);
		}

		if (values.size() < 5)
		{
			continue;
		}

		string Name = values[5];

		int table = FromHex(values[0]);
		if (values.size() > 6)
		{
			Name += " : ";
			Name += values[6];
		}
		EntranceTable.push_back(Name);

		if (values.size() > 7)
		{
			int PlaceNumber = strtol(values[7].c_str(), nullptr, 10);
			if (PlaceTable.find(PlaceNumber) != PlaceTable.end())
			{
				Log::Error("Place %d already in the table!", PlaceNumber);
			}
			else
			{
				PlaceTable[PlaceNumber] = Name;
			}
			PlaceNumTable.push_back(PlaceNumber);
		}
		else
		{
			PlaceNumTable.push_back(9999);
		}
	}*/
}