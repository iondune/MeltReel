

#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <iomanip>

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
	Line(2, "<property name='resource'>&lt;producer&gt;</property>");
	Line(2, "<property name='aspect_ratio'>1</property>");
	Line(2, "<property name='mlt_service'>noise</property>");
	Line(2, "<filter id='zeroVolume'>");
	Line(3, "<property name='window'>75</property>");
	Line(3, "<property name='max_gain'>20dB</property>");
	Line(3, "<property name='mlt_type'>filter</property>");
	Line(3, "<property name='mlt_service'>volume</property>");
	Line(3, "<property name='gain'>0</property>");
	Line(3, "<filter id='zeroColor'>");
	Line(4, "<property name='gamma'>0</property>");
	Line(4, "<property name='mlt_type'>filter</property>");
	Line(4, "<property name='mlt_service'>gamma</property>");
	Line(3, "</filter>");
	Line(2, "</filter>");
	Line(1, "</producer>");
}

void WriteSourceClip(string const & source)
{
	Line(1, tfm::format("<producer id='source_%s'>", source));
	Line(2, "<property name='mlt_type'>producer</property>");
	Line(2, tfm::format("<property name='resource'>../Raw/%s.mkv</property>", source));
	Line(1, "</producer>");
}

void WriteTransitionOut(int const id, string const & source, int const frame)
{
	Line(1, tfm::format("<tractor id='%dout' in='0' out='24'>", id));
	Line(2, tfm::format("<track producer='source_%s' in='%d' out='%d'/>", source, frame, frame + 24));
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

void WriteTransitionIn(int const id, string const & source, int const frame)
{
	Line(1, tfm::format("<tractor id='%din' in='0' out='24'>", id));
	Line(2, "<track producer='black' in='0' out='24'/>");
	Line(2, tfm::format("<track producer='source_%s' in='%d' out='%d'/>", source, frame, frame + 24));
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
	string Source = "none";
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

string FramestoHMS(int const Frames)
{
	const int Hours = Frames / (60 * 60 * frameRate);
	const int Minutes = Frames % (60 * 60 * frameRate) / (60 * frameRate);
	const int Seconds = Frames % (60 * frameRate) / frameRate;

	stringstream res;
	res << setfill('0') << setw(2) << Hours << ":"
		<< setfill('0') << setw(2) << Minutes << ":"
		<< setfill('0') << setw(2) << Seconds;
	return res.str();
}

vector<Clip> ReadClipsFromFile(ifstream & input)
{
	vector<Clip> clips;

	char const field_delim = '\t';

	int index = 0;


	for (string row; getline(input, row); )
	{
		vector<string> values;
		istringstream ss(row);

		for (string field; getline(ss, field, field_delim); )
		{
			values.push_back(field);
		}

		if (values.size() != 3)
		{
			cerr << "Malformed row at index " << index << endl;
			continue;
		}

		Clip c;
		c.Source = values[0];
		c.Start = HMStoFrames(values[1]);
		c.End = HMStoFrames(values[2]);

		if (c.Start > c.End)
		{
			swap(c.Start, c.End);
			cerr << "Found record with mismatched times at index " << index << endl;
		}

		clips.push_back(c);

		index ++;
	}

	return clips;
}

int main(int argc, char ** argv)
{
	string infileName, outfileName;

	if (argc == 3)
	{
		infileName = argv[1];
		outfileName = argv[2];
	}
	else
	{
		cerr << "Expected 2 arguments, found " << argc - 1 << "." << endl;
		cerr << "usage: ./MeltReel <infile> <outfile>" << endl;
		return 1;
	}

	ifstream input(infileName);
	if (! input.is_open())
	{
		cerr << "Input file could not be opened: " << infileName << endl;
		return 1;
	}

	outfile.open(outfileName);
	if (! outfile.is_open())
	{
		cerr << "Output file could not be opened: " << outfileName << endl;
		return 1;
	}


	vector<Clip> clips = ReadClipsFromFile(input);

	WriteHeader();
	WriteBlackClip();
	WriteSourceClip("p1");
	WriteSourceClip("p2");
	WriteSourceClip("p3");
	WriteSourceClip("p4");
	WriteSourceClip("p5");
	WriteSourceClip("p6");
	WriteSourceClip("p7");
	WriteSourceClip("p8");
	WriteSourceClip("p9");
	WriteSourceClip("p10");
	WriteSourceClip("p11");
	WriteSourceClip("p12");
	WriteSourceClip("p13");
	WriteSourceClip("p14");
	WriteSourceClip("p15");
	WriteSourceClip("p16");
	WriteSourceClip("p17");
	WriteSourceClip("p18");
	WriteSourceClip("p19");
	WriteSourceClip("p20");
	WriteSourceClip("p21");
	WriteSourceClip("p22");
	WriteSourceClip("p23");
	WriteSourceClip("p23.1");
	WriteSourceClip("p24");
	WriteSourceClip("p25");
	WriteSourceClip("p26");
	WriteSourceClip("p27");
	WriteSourceClip("p28");
	WriteSourceClip("p29");
	WriteSourceClip("p30");
	WriteSourceClip("p31");
	WriteSourceClip("p32");
	WriteSourceClip("p33");
	WriteSourceClip("p34");
	WriteSourceClip("p35");
	WriteSourceClip("p36");
	WriteSourceClip("p37");
	WriteSourceClip("p38");
	WriteSourceClip("p39");
	WriteSourceClip("p40");
	WriteSourceClip("p41");
	WriteSourceClip("p42");
	WriteSourceClip("p43");
	WriteSourceClip("p44");
	WriteSourceClip("p45");

	int totalFrames = 0;

	for (int i = 0; i < clips.size(); ++ i)
	{
		auto clip = clips[i];

		WriteTransitionIn(i, clip.Source, clip.Start);
		WriteTransitionOut(i, clip.Source, clip.End - 24);

		totalFrames += clip.End - clip.Start;
	}

	Line(1, "<playlist id='playlist0'>");
	for (int i = 0; i < clips.size(); ++ i)
	{
		auto clip = clips[i];

		Line(2, tfm::format("<entry producer='%din' in='0' out='24'/>", i));
		Line(2, tfm::format("<entry producer='source_%s' in='%d' out='%d'/>", clip.Source, clip.Start + 24 + 1, clip.End - 24 - 1));
		Line(2, tfm::format("<entry producer='%dout' in='0' out='24'/>", i));
	}
	Line(1, "</playlist>");

	Line(1, "<tractor id='tractor2' title='clip1.mkv' global_feed='1'>");
	Line(2, "<track producer='playlist0'/>");
	Line(1, "</tractor>");
	Line("</mlt>");

	cout << "Wrote a Melt configuration of duration " << FramestoHMS(totalFrames) << " (" << totalFrames << " frames) with " << clips.size() << " clips." << endl;

	return 0;
}
