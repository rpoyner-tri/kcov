#include <reporter.hh>
#include <elf.hh>
#include <configuration.hh>
#include <writer.hh>
#include <filter.hh>
#include <utils.hh>

#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdio.h>
#include <string>
#include <list>
#include <unordered_map>
#include <thread>

using namespace kcov;


struct summaryStruct
{
	uint32_t nLines;
	uint32_t nExecutedLines;
	char name[256];
};

static const uint8_t icon_ruby[] =
  { 0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x0d,
0x49, 0x48, 0x44, 0x52, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x01, 0x03, 0x00, 0x00, 0x00,
0x25, 0xdb, 0x56, 0xca, 0x00, 0x00, 0x00, 0x07, 0x74, 0x49, 0x4d, 0x45, 0x07, 0xd2, 0x07, 0x11, 0x0f,
0x18, 0x10, 0x5d, 0x57, 0x34, 0x6e, 0x00, 0x00, 0x00, 0x09, 0x70, 0x48, 0x59, 0x73, 0x00, 0x00, 0x0b,
0x12, 0x00, 0x00, 0x0b, 0x12, 0x01, 0xd2, 0xdd, 0x7e, 0xfc, 0x00, 0x00, 0x00, 0x04, 0x67, 0x41, 0x4d,
0x41, 0x00, 0x00, 0xb1, 0x8f, 0x0b, 0xfc, 0x61, 0x05, 0x00, 0x00, 0x00, 0x06, 0x50, 0x4c, 0x54, 0x45,
0xff, 0x35, 0x2f, 0x00, 0x00, 0x00, 0xd0, 0x33, 0x9a, 0x9d, 0x00, 0x00, 0x00, 0x0a, 0x49, 0x44, 0x41,
0x54, 0x78, 0xda, 0x63, 0x60, 0x00, 0x00, 0x00, 0x02, 0x00, 0x01, 0xe5, 0x27, 0xde, 0xfc, 0x00, 0x00,
0x00, 0x00, 0x49, 0x45, 0x4e, 0x44, 0xae, 0x42, 0x60, 0x82 };

static const uint8_t icon_amber[] =
  { 0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x0d,
0x49, 0x48, 0x44, 0x52, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x01, 0x03, 0x00, 0x00, 0x00,
0x25, 0xdb, 0x56, 0xca, 0x00, 0x00, 0x00, 0x07, 0x74, 0x49, 0x4d, 0x45, 0x07, 0xd2, 0x07, 0x11, 0x0f,
0x28, 0x04, 0x98, 0xcb, 0xd6, 0xe0, 0x00, 0x00, 0x00, 0x09, 0x70, 0x48, 0x59, 0x73, 0x00, 0x00, 0x0b,
0x12, 0x00, 0x00, 0x0b, 0x12, 0x01, 0xd2, 0xdd, 0x7e, 0xfc, 0x00, 0x00, 0x00, 0x04, 0x67, 0x41, 0x4d,
0x41, 0x00, 0x00, 0xb1, 0x8f, 0x0b, 0xfc, 0x61, 0x05, 0x00, 0x00, 0x00, 0x06, 0x50, 0x4c, 0x54, 0x45,
0xff, 0xe0, 0x50, 0x00, 0x00, 0x00, 0xa2, 0x7a, 0xda, 0x7e, 0x00, 0x00, 0x00, 0x0a, 0x49, 0x44, 0x41,
0x54, 0x78, 0xda, 0x63, 0x60, 0x00, 0x00, 0x00, 0x02, 0x00, 0x01, 0xe5, 0x27, 0xde, 0xfc, 0x00, 0x00,
0x00, 0x00, 0x49, 0x45, 0x4e, 0x44, 0xae, 0x42, 0x60, 0x82 };

static const uint8_t icon_emerald[] =
  { 0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x0d,
0x49, 0x48, 0x44, 0x52, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x01, 0x03, 0x00, 0x00, 0x00, 0x25,
0xdb, 0x56, 0xca, 0x00, 0x00, 0x00, 0x07, 0x74, 0x49, 0x4d, 0x45, 0x07, 0xd2, 0x07, 0x11, 0x0f, 0x22, 0x2b,
0xc9, 0xf5, 0x03, 0x33, 0x00, 0x00, 0x00, 0x09, 0x70, 0x48, 0x59, 0x73, 0x00, 0x00, 0x0b, 0x12, 0x00, 0x00,
0x0b, 0x12, 0x01, 0xd2, 0xdd, 0x7e, 0xfc, 0x00, 0x00, 0x00, 0x04, 0x67, 0x41, 0x4d, 0x41, 0x00, 0x00, 0xb1,
0x8f, 0x0b, 0xfc, 0x61, 0x05, 0x00, 0x00, 0x00, 0x06, 0x50, 0x4c, 0x54, 0x45, 0x1b, 0xea, 0x59, 0x0a, 0x0a,
0x0a, 0x0f, 0xba, 0x50, 0x83, 0x00, 0x00, 0x00, 0x0a, 0x49, 0x44, 0x41, 0x54, 0x78, 0xda, 0x63, 0x60, 0x00,
0x00, 0x00, 0x02, 0x00, 0x01, 0xe5, 0x27, 0xde, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4e, 0x44, 0xae,
0x42, 0x60, 0x82 };

static const uint8_t icon_snow[] =
  { 0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x0d,
0x49, 0x48, 0x44, 0x52, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x01, 0x03, 0x00, 0x00, 0x00,
0x25, 0xdb, 0x56, 0xca, 0x00, 0x00, 0x00, 0x07, 0x74, 0x49, 0x4d, 0x45, 0x07, 0xd2, 0x07, 0x11, 0x0f,
0x1e, 0x1d, 0x75, 0xbc, 0xef, 0x55, 0x00, 0x00, 0x00, 0x09, 0x70, 0x48, 0x59, 0x73, 0x00, 0x00, 0x0b,
0x12, 0x00, 0x00, 0x0b, 0x12, 0x01, 0xd2, 0xdd, 0x7e, 0xfc, 0x00, 0x00, 0x00, 0x04, 0x67, 0x41, 0x4d,
0x41, 0x00, 0x00, 0xb1, 0x8f, 0x0b, 0xfc, 0x61, 0x05, 0x00, 0x00, 0x00, 0x06, 0x50, 0x4c, 0x54, 0x45,
0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x55, 0xc2, 0xd3, 0x7e, 0x00, 0x00, 0x00, 0x0a, 0x49, 0x44, 0x41,
0x54, 0x78, 0xda, 0x63, 0x60, 0x00, 0x00, 0x00, 0x02, 0x00, 0x01, 0xe5, 0x27, 0xde, 0xfc, 0x00, 0x00,
0x00, 0x00, 0x49, 0x45, 0x4e, 0x44, 0xae, 0x42, 0x60, 0x82 };

static const uint8_t icon_glass[] =
  { 0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x0d,
0x49, 0x48, 0x44, 0x52, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x01, 0x03, 0x00, 0x00, 0x00,
0x25, 0xdb, 0x56, 0xca, 0x00, 0x00, 0x00, 0x04, 0x67, 0x41, 0x4d, 0x41, 0x00, 0x00, 0xb1, 0x8f, 0x0b,
0xfc, 0x61, 0x05, 0x00, 0x00, 0x00, 0x06, 0x50, 0x4c, 0x54, 0x45, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00,
0x55, 0xc2, 0xd3, 0x7e, 0x00, 0x00, 0x00, 0x01, 0x74, 0x52, 0x4e, 0x53, 0x00, 0x40, 0xe6, 0xd8, 0x66,
0x00, 0x00, 0x00, 0x01, 0x62, 0x4b, 0x47, 0x44, 0x00, 0x88, 0x05, 0x1d, 0x48, 0x00, 0x00, 0x00, 0x09,
0x70, 0x48, 0x59, 0x73, 0x00, 0x00, 0x0b, 0x12, 0x00, 0x00, 0x0b, 0x12, 0x01, 0xd2, 0xdd, 0x7e, 0xfc,
0x00, 0x00, 0x00, 0x07, 0x74, 0x49, 0x4d, 0x45, 0x07, 0xd2, 0x07, 0x13, 0x0f, 0x08, 0x19, 0xc4, 0x40,
0x56, 0x10, 0x00, 0x00, 0x00, 0x0a, 0x49, 0x44, 0x41, 0x54, 0x78, 0x9c, 0x63, 0x60, 0x00, 0x00, 0x00,
0x02, 0x00, 0x01, 0x48, 0xaf, 0xa4, 0x71, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4e, 0x44, 0xae, 0x42,
0x60, 0x82 };

static const char css_text[] = "/* Based upon the lcov CSS style, style files can be reused */\n"
		"body { color: #000000; background-color: #FFFFFF; }\n"
		"a:link { color: #284FA8; text-decoration: underline; }\n"
		"a:visited { color: #00CB40; text-decoration: underline; }\n"
		"a:active { color: #FF0040; text-decoration: underline; }\n"
		"td.title { text-align: center; padding-bottom: 10px; font-size: 20pt; font-weight: bold; }\n"
		"td.ruler { background-color: #6688D4; }\n"
		"td.headerItem { text-align: right; padding-right: 6px; font-family: sans-serif; font-weight: bold; }\n"
		"td.headerValue { text-align: left; color: #284FA8; font-family: sans-serif; font-weight: bold; }\n"
		"td.versionInfo { text-align: center; padding-top:  2px; }\n"
		"pre.source { font-family: monospace; white-space: pre; }\n"
		"span.lineNum { background-color: #EFE383; }\n"
		"span.lineCov { background-color: #CAD7FE; }\n"
		"span.linePartCov { background-color: #FFEA20; }\n"
		"span.lineNoCov { background-color: #FF6230; }\n"
		"td.tableHead { text-align: center; color: #FFFFFF; background-color: #6688D4; font-family: sans-serif; font-size: 120%; font-weight: bold; }\n"
		"td.coverFile { text-align: left; padding-left: 10px; padding-right: 20px; color: #284FA8; background-color: #DAE7FE; font-family: monospace; }\n"
		"td.coverBar { padding-left: 10px; padding-right: 10px; background-color: #DAE7FE; }\n"
		"td.coverBarOutline { background-color: #000000; }\n"
		"td.coverPerHi { text-align: right; padding-left: 10px; padding-right: 10px; background-color: #A7FC9D; font-weight: bold; }\n"
		"td.coverPerLeftMed { text-align: left; padding-left: 10px; padding-right: 10px; background-color: #FFEA20; font-weight: bold; }\n"
		"td.coverPerLeftLo { text-align: left; padding-left: 10px; padding-right: 10px; background-color: #FF0000; font-weight: bold; }\n"
		"td.coverPerLeftHi { text-align: left; padding-left: 10px; padding-right: 10px; background-color: #A7FC9D; font-weight: bold; }\n"
		"td.coverNumHi { text-align: right; padding-left: 10px; padding-right: 10px; background-color: #A7FC9D; }\n"
		"td.coverPerMed { text-align: right; padding-left: 10px; padding-right: 10px; background-color: #FFEA20; font-weight: bold; }\n"
		"td.coverNumMed { text-align: right; padding-left: 10px; padding-right: 10px; background-color: #FFEA20; }\n"
		"td.coverPerLo { text-align: right; padding-left: 10px; padding-right: 10px; background-color: #FF0000; font-weight: bold; }\n"
		"td.coverNumLo { text-align: right; padding-left: 10px; padding-right: 10px; background-color: #FF0000; }\n";


class HtmlWriter : public IWriter, public IElf::IListener
{
public:
	HtmlWriter(IElf &elf, IReporter &reporter) :
		m_elf(elf), m_reporter(reporter), m_filter(IFilter::getInstance()),
		m_stop(false), m_thread(NULL)
	{
		IConfiguration &conf = IConfiguration::getInstance();
		m_indexDirectory = conf.getOutDirectory();
		m_outDirectory = m_indexDirectory + conf.getBinaryName() + "/";
		m_dbFileName = m_outDirectory + "coverage.db";
		m_summaryDbFileName = m_outDirectory + "summary.db";
		m_commonPath = "not set";

		m_elf.registerListener(*this);

	}

	virtual ~HtmlWriter()
	{
		stop();
		if (m_thread)
			delete m_thread;
	}

	void start()
	{
		size_t sz;
		void *data = read_file(&sz, m_dbFileName.c_str());

		if (data) {
			m_reporter.unMarshal(data, sz);

			free(data);
		}

		m_thread = new std::thread(threadMainStatic, this);
		m_cv.notify_all();
	}

	void stop()
	{
		m_stop = true;
		m_cv.notify_all();

		if (m_thread)
			m_thread->join();
		std::this_thread::sleep_for(std::chrono::milliseconds(100));

		size_t sz;
		void *data = m_reporter.marshal(&sz);

		if (data)
			write_file(data, sz, m_dbFileName.c_str());

		free(data);
	}

private:
	class File
	{
	public:
		typedef std::unordered_map<unsigned int, std::string> LineMap_t;

		File(const char *filename) :
			m_name(filename), m_codeLines(0), m_executedLines(0), m_lastLineNr(0)
		{
			size_t pos = m_name.rfind('/');

			if (pos != std::string::npos)
				m_fileName = m_name.substr(pos + 1, std::string::npos);
			else
				m_fileName = m_name;
			m_outFileName = m_fileName + ".html";

			readFile(filename);
		}

		std::string m_name;
		std::string m_fileName;
		std::string m_outFileName;
		LineMap_t m_lineMap;
		unsigned int m_codeLines;
		unsigned int m_executedLines;
		unsigned int m_lastLineNr;

	private:
		void readFile(const char *filename)
		{
			FILE *fp = fopen(filename, "r");
			unsigned int lineNr = 1;

			panic_if(!fp, "Can't open %s", filename);

			while (1)
			{
				char *lineptr = NULL;
				ssize_t res;
				size_t n;

				res = getline(&lineptr, &n, fp);
				if (res < 0)
					break;
				m_lineMap[lineNr] = std::string(lineptr);

				free((void *)lineptr);
				lineNr++;
			}

			m_lastLineNr = lineNr;

			fclose(fp);
		}
	};

	typedef std::unordered_map<std::string, File *> FileMap_t;


	/* Called when the ELF is parsed */
	void onLine(const char *file, unsigned int lineNr, unsigned long addr)
	{
		if (m_files.find(std::string(file)) != m_files.end())
			return;

		if (!file_exists(file))
			return;

		m_files[std::string(file)] = new File(file);
	}

	void writeOne(File *file)
	{
		std::string outName = m_outDirectory + "/" + file->m_outFileName;
		unsigned int nExecutedLines = 0;
		unsigned int nCodeLines = 0;

		std::string s =
				"<pre class=\"source\">\n";
		for (unsigned int n = 1; n < file->m_lastLineNr; n++) {
			std::string line = file->m_lineMap[n];

			s += "<span class=\"lineNum\">" + fmt("%5u", n) + "</span>";

			if (!m_reporter.lineIsCode(file->m_name.c_str(), n)) {
				s += "              : " + escapeHtml(line) + "</span>\n";
				continue;
			}

			IReporter::LineExecutionCount cnt =
					m_reporter.getLineExecutionCount(file->m_name.c_str(), n);

			std::string lineClass = "lineNoCov";

			nExecutedLines += !!cnt.m_hits;
			nCodeLines++;

			if (cnt.m_hits == cnt.m_possibleHits)
				lineClass = "lineCov";
			else if (cnt.m_hits)
				lineClass = "linePartCov";

			s += "<span class=\"" + lineClass + "\">    " +
					fmt("%3u", cnt.m_hits) + "  / " + fmt("%3u", cnt.m_possibleHits) + ": " +
					escapeHtml(line) +
					"</span>\n";
		}
		s += "</pre>\n";

		s = getHeader(nCodeLines, nExecutedLines) + s + getFooter(file);

		write_file((void *)s.c_str(), s.size(), outName.c_str());

		// Update the execution count
		file->m_executedLines = nExecutedLines;
		file->m_codeLines = nCodeLines;
	}

	void writeIndex()
	{
		std::string s;
		unsigned int nTotalExecutedLines = 0;
		unsigned int nTotalCodeLines = 0;

		for (FileMap_t::iterator it = m_files.begin();
				it != m_files.end();
				it++) {
			File *file = it->second;
			unsigned int nExecutedLines = file->m_executedLines;
			unsigned int nCodeLines = file->m_codeLines;
			double percent = 0;

			if (m_filter.runFilters(file->m_name) == false)
				continue;

			if (nCodeLines != 0)
				percent = ((double)nExecutedLines / (double)nCodeLines) * 100;

			nTotalCodeLines += nCodeLines;
			nTotalExecutedLines += nExecutedLines;

			std::string coverPer = strFromPercentage(percent);

			std::string listName = file->m_name;

			size_t pos = listName.find(m_commonPath);
			unsigned int stripLevel = IConfiguration::getInstance().getPathStripLevel();

			if (pos != std::string::npos && m_commonPath.size() != 0 && stripLevel != ~0U) {
				std::string pathToRemove = m_commonPath;

				for (unsigned int i = 0; i < stripLevel; i++) {
					size_t slashPos = pathToRemove.rfind("/");

					if (slashPos == std::string::npos)
						break;
					pathToRemove = pathToRemove.substr(0, slashPos);
				}

				std::string prefix = "[...]";

				if (pathToRemove == "")
					prefix = "";
				listName = prefix + listName.substr(pathToRemove.size());
			}

			s +=
				"    <tr>\n"
				"      <td class=\"coverFile\"><a href=\"" + file->m_outFileName + "\" title=\"" + file->m_fileName + "\">" + listName + "</a></td>\n"
				"      <td class=\"coverBar\" align=\"center\">\n"
				"        <table border=\"0\" cellspacing=\"0\" cellpadding=\"1\"><tr><td class=\"coverBarOutline\">" + constructBar(percent) + "</td></tr></table>\n"
				"      </td>\n"
				"      <td class=\"coverPer" + coverPer + "\">" + fmt("%.1f", percent) + "&nbsp;%</td>\n"
				"      <td class=\"coverNum" + coverPer + "\">" + fmt("%u", nExecutedLines) + "&nbsp;/&nbsp;" + fmt("%u", nCodeLines) + "&nbsp;lines</td>\n"
				"    </tr>\n";
		}

		s = getHeader(nTotalCodeLines, nTotalExecutedLines) + getIndexHeader() + s + getFooter(NULL);

		write_file((void *)s.c_str(), s.size(), (m_outDirectory + "index.html").c_str());

		// Produce a summary
		IReporter::ExecutionSummary summary = m_reporter.getExecutionSummary();
		size_t sz;

		void *data = marshalSummary(summary,
				IConfiguration::getInstance().getBinaryName(), &sz);

		if (data)
			write_file(data, sz, "%s", m_summaryDbFileName.c_str());

		free(data);
	}

	void writeGlobalIndex()
	{
		DIR *dir;
		struct dirent *de;
		std::string idx = m_indexDirectory.c_str();
		std::string s;
		unsigned int nTotalExecutedLines = 0;
		unsigned int nTotalCodeLines = 0;

		dir = opendir(idx.c_str());
		panic_if(!dir, "Can't open directory %s\n", idx.c_str());

		for (de = readdir(dir); de; de = readdir(dir)) {
			std::string cur = idx + de->d_name + "/summary.db";

			if (!file_exists(cur.c_str()))
				continue;

			size_t sz;
			void *data = read_file(&sz, "%s", cur.c_str());

			if (!data)
				continue;

			IReporter::ExecutionSummary summary;
			std::string name;
			bool res = unMarshalSummary(data, sz, summary, name);
			free(data);

			if (!res)
				continue;


			double percent = 0;

			if (summary.m_lines != 0)
				percent = (summary.m_executedLines / (double)summary.m_lines) * 100;
			nTotalCodeLines += summary.m_lines;
			nTotalExecutedLines += summary.m_executedLines;

			std::string coverPer = strFromPercentage(percent);

			s +=
					"    <tr>\n"
					"      <td class=\"coverFile\"><a href=\"" + std::string(de->d_name) + "/index.html\" title=\"" + name + "\">" + name + "</a></td>\n"
					"      <td class=\"coverBar\" align=\"center\">\n"
					"        <table border=\"0\" cellspacing=\"0\" cellpadding=\"1\"><tr><td class=\"coverBarOutline\">" + constructBar(percent) + "</td></tr></table>\n"
					"      </td>\n"
					"      <td class=\"coverPer" + coverPer + "\">" + fmt("%.1f", percent) + "&nbsp;%</td>\n"
					"      <td class=\"coverNum" + coverPer + "\">" + fmt("%u", summary.m_executedLines) + "&nbsp;/&nbsp;" + fmt("%u", summary.m_lines) + "&nbsp;lines</td>\n"
					"    </tr>\n";

		}
		s = getHeader(nTotalCodeLines, nTotalExecutedLines) + getIndexHeader() + s + getFooter(NULL);

		write_file((void *)s.c_str(), s.size(), (m_indexDirectory + "index.html").c_str());
	}

	void *marshalSummary(IReporter::ExecutionSummary &summary,
			std::string &name, size_t *sz)
	{
		struct summaryStruct *p;

		p = (struct summaryStruct *)xmalloc(sizeof(struct summaryStruct));
		memset(p, 0, sizeof(*p));

		p->nLines = summary.m_lines;
		p->nExecutedLines = summary.m_executedLines;
		strncpy(p->name, name.c_str(), sizeof(p->name) - 1);

		*sz = sizeof(*p);

		return (void *)p;
	}

	bool unMarshalSummary(void *data, size_t sz,
			IReporter::ExecutionSummary &summary,
			std::string &name)
	{
		struct summaryStruct *p = (struct summaryStruct *)data;

		if (sz != sizeof(*p))
			return false;

		summary.m_lines = p->nLines;
		summary.m_executedLines = p->nExecutedLines;
		name = std::string(p->name);

		return true;
	}


	void write()
	{
		for (FileMap_t::iterator it = m_files.begin();
				it != m_files.end();
				it++) {
			File *file = it->second;

			if (m_filter.runFilters(file->m_name) == false)
				continue;

			writeOne(file);
		}

		writeIndex();

		writeGlobalIndex();
	}

	std::string strFromPercentage(double percent)
	{
		IConfiguration &conf = IConfiguration::getInstance();
		std::string coverPer = "Med";

		if (percent >= conf.getHighLimit())
			coverPer = "Hi";
		else if (percent < conf.getLowLimit())
			coverPer = "Lo";

		return coverPer;
	}

	std::string getIndexHeader()
	{
		return std::string(
				"<center>\n"
				"  <table width=\"80%\" cellpadding=\"2\" cellspacing=\"1\" border=\"0\">\n"
				"    <tr>\n"
				"      <td width=\"50%\"><br/></td>\n"
				"      <td width=\"15%\"></td>\n"
				"      <td width=\"15%\"></td>\n"
				"      <td width=\"20%\"></td>\n"
				"    </tr>\n"
				"    <tr>\n"
				"      <td class=\"tableHead\">Filename</td>\n"
				"      <td class=\"tableHead\" colspan=\"3\">Coverage</td>\n"
				"    </tr>\n"
		);
	}

	std::string getFooter(File *file)
	{
		return std::string(
				"<table width=\"100%\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\">\n"
				"  <tr><td class=\"ruler\"><img src=\"glass.png\" width=\"3\" height=\"3\" alt=\"\"/></td></tr>\n"
				"  <tr><td class=\"versionInfo\">Generated by: <a href=\"http://simonkagstrom.github.com/kcov/index.html\">Kcov</a> (based on <a href=\"http://bcov.sourceforge.net\">bcov</a>)</td></tr>\n"
				"</table>\n"
				"<br/>\n"
				"</body>\n"
				"</html>\n"
				);
	}

	std::string getHeader(unsigned int nCodeLines, unsigned int nExecutedLines)
	{
		IConfiguration &conf = IConfiguration::getInstance();
		std::string percentage_text = "Lo";
		double percentage = 0;
		char date_buf[80];
		time_t t;
		struct tm *tm;

		if (nCodeLines != 0)
			percentage = ((double)nExecutedLines / (double)nCodeLines) * 100;

		if (percentage > conf.getLowLimit() && percentage < conf.getHighLimit())
			percentage_text = "Med";
		else if (percentage >= conf.getHighLimit())
			percentage_text = "Hi";

		t = time(NULL);
		tm = localtime(&t);
		strftime(date_buf, sizeof(date_buf), "%Y-%m-%d %H:%M:%S", tm);

		std::string date(date_buf);
		std::string instrumentedLines = fmt("%u", nCodeLines);
		std::string lines = fmt("%u", nExecutedLines);
		std::string covered = fmt("%.1f", percentage);

		return std::string(
				"<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\">\n"
				"<html>\n"
				"<head>\n"
				"  <title>Coverage - " + escapeHtml(conf.getBinaryName()) + "</title>\n"
				"  <link rel=\"stylesheet\" type=\"text/css\" href=\"bcov.css\"/>\n"
				"</head>\n"
				"<body>\n"
				"<table width=\"100%\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\">\n"
				"  <tr><td class=\"title\">Coverage Report</td></tr>\n"
				"  <tr><td class=\"ruler\"><img src=\"glass.png\" width=\"3\" height=\"3\" alt=\"\"/></td></tr>\n"
				"  <tr>\n"
				"    <td width=\"100%\">\n"
				"      <table cellpadding=\"1\" border=\"0\" width=\"100%\">\n"
				"        <tr>\n"
				"          <td class=\"headerItem\" width=\"20%\">Command:</td>\n"
				"          <td class=\"headerValue\" width=\"80%\" colspan=6>" + escapeHtml(conf.getBinaryName()) + "</td>\n"
				"        </tr>\n"
				"        <tr>\n"
				"          <td class=\"headerItem\" width=\"20%\">Date:</td>\n"
				"          <td class=\"headerValue\" width=\"15%\">" + escapeHtml(date) + "</td>\n"
				"          <td width=\"5%\"></td>\n"
				"          <td class=\"headerItem\" width=\"20%\">Instrumented&nbsp;lines:</td>\n"
				"          <td class=\"headerValue\" width=\"10%\">" + instrumentedLines + "</td>\n"
				"        </tr>\n"
				"        <tr>\n"
				"          <td class=\"headerItem\" width=\"20%\">Code&nbsp;covered:</td>\n"
				"          <td class=\"coverPerLeft" + percentage_text + "\" width=\"15%\">" + covered + "%</td>\n"
				"          <td width=\"5%\"></td>\n"
				"          <td class=\"headerItem\" width=\"20%\">Executed&nbsp;lines:</td>\n"
				"          <td class=\"headerValue\" width=\"10%\">" + lines + "</td>\n"
				"        </tr>\n"
				"      </table>\n"
				"    </td>\n"
				"  </tr>\n"
				"  <tr><td class=\"ruler\"><img src=\"glass.png\" width=\"3\" height=\"3\" alt=\"\"/></td></tr>\n"
				"</table>\n"
				);
	}

	std::string fmt(const char *fmt, ...)
	{
		char buf[4096];
		va_list ap;
		int res;

		va_start(ap, fmt);
		res = vsnprintf(buf, sizeof(buf), fmt, ap);
		va_end(ap);

		panic_if(res >= (int)sizeof(buf),
				"Buffer overflow");

		return std::string(buf);
	}

	std::string constructBar(double percent)
	{
		const char* color = "ruby.png";
		char buf[1024];
		int width = (int)(percent+0.5);
		IConfiguration &conf = IConfiguration::getInstance();

		if (percent >= conf.getHighLimit())
			color = "emerald.png";
		else if (percent > conf.getLowLimit())
			color = "amber.png";
		else if (percent <= 1)
			color = "snow.png";

		xsnprintf(buf, sizeof(buf),
				"<img src=\"%s\" width=\"%d\" height=\"10\" alt=\"%.1f%%\"/><img src=\"snow.png\" width=\"%d\" height=\"10\" alt=\"%.1f%%\"/>",
				color, width, percent, 100 - width, percent);

		return std::string(buf);
	}

	char *escapeHelper(char *dst, const char *what)
	{
		int len = strlen(what);

		strcpy(dst, what);

		return dst + len;
	}

	std::string escapeHtml(std::string &str)
	{
		const char *s = str.c_str();
		char buf[4096];
		char *dst = buf;
		size_t len = strlen(s);
		size_t i;

		memset(buf, 0, sizeof(buf));
		for (i = 0; i < len; i++) {
			char c = s[i];

			switch (c) {
			case '<':
				dst = escapeHelper(dst, "&lt;");
				break;
			case '>':
				dst = escapeHelper(dst, "&gt;");
				break;
			case '&':
				dst = escapeHelper(dst, "&amp;");
				break;
			case '\"':
				dst = escapeHelper(dst, "&quot;");
				break;
			case '\'':
				dst = escapeHelper(dst, "&#039;");
				break;
			case '/':
				dst = escapeHelper(dst, "&#047;");
				break;
			case '\\':
				dst = escapeHelper(dst, "&#092;");
				break;
			case '\n': case '\r':
				dst = escapeHelper(dst, " ");
				break;
			default:
				*dst = c;
				dst++;
				break;
			}
		}

		return std::string(buf);
	}

	void writeHelperFiles(std::string dir)
	{
		write_file(icon_ruby, sizeof(icon_ruby), "%s/ruby.png", dir.c_str());
		write_file(icon_amber, sizeof(icon_amber), "%s/amber.png", dir.c_str());
		write_file(icon_emerald, sizeof(icon_emerald), "%s/emerald.png", dir.c_str());
		write_file(icon_snow, sizeof(icon_snow), "%s/snow.png", dir.c_str());
		write_file(icon_glass, sizeof(icon_glass), "%s/glass.png", dir.c_str());
		write_file(css_text, sizeof(css_text), "%s/bcov.css", dir.c_str());
	}

	void threadMain()
	{
		mkdir(m_indexDirectory.c_str(), 0755);
		mkdir(m_outDirectory.c_str(), 0755);

		writeHelperFiles(m_indexDirectory);
		writeHelperFiles(m_outDirectory);

		for (FileMap_t::iterator it = m_files.begin();
				it != m_files.end();
				it++) {
			File *file = it->second;

			if (m_filter.runFilters(file->m_name) == false)
				continue;

			if (m_commonPath == "not set")
				m_commonPath = file->m_name;

			/* Already matching? */
			if (file->m_name.find(m_commonPath) == 0)
				continue;

			while (1) {
				size_t pos = m_commonPath.rfind('/');
				if (pos == std::string::npos)
					break;

				m_commonPath = m_commonPath.substr(0, pos);
				if (file->m_name.find(m_commonPath) == 0)
					break;
			}
		}

		while (!m_stop) {
			std::unique_lock<std::mutex> lk(m_mutex);

			m_cv.wait_for(lk, std::chrono::milliseconds(1003));

			write();
		}
	}

	static void threadMainStatic(HtmlWriter *writer)
	{
		writer->threadMain();
	}

	IElf &m_elf;
	IReporter &m_reporter;
	IFilter &m_filter;
	FileMap_t m_files;

	bool m_stop;
	std::string m_outDirectory;
	std::string m_indexDirectory;
	std::string m_dbFileName;
	std::string m_summaryDbFileName;
	std::thread *m_thread;
	std::condition_variable m_cv;
	std::mutex m_mutex;
	std::string m_commonPath;
};

IWriter &IWriter::create(IElf &elf, IReporter &reporter)
{
	return *new HtmlWriter(elf, reporter);
}
