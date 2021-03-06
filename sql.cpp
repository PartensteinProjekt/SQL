// ----------------------------------------------------------------------------------------------------------------------
#include <sql.h>
// ----------------------------------------------------------------------------------------------------------------------

#include <sqlite_wrapper.h>

using std::string;
using std::unique_ptr;
using std::make_unique;
using std::initializer_list;
using std::ostream;
using std::vector;

#include <algorithm>
using std::replace;

#include <fstream>
using std::ifstream;
using std::ofstream;
using std::getline;

#include <filesystem>
namespace fs = std::experimental::filesystem;

// ----------------------------------------------------------------------------------------------------------------------
unique_ptr<sql_wrapper> sql::create_database(const string& db_name, initializer_list<string> table_names, ostream& os)
// ----------------------------------------------------------------------------------------------------------------------
{
    auto db = make_unique<sql_wrapper>(db_name);

    for (auto name : table_names) {
        auto [_, error] = db->execute_cmd(sql::file_to_string(name));
        if (error) {
            os << "ERROR: " << error.value() << '\n';
            throw std::runtime_error("bad query during 'create_db'");
        }
    }

    return std::move(db);
}

// ----------------------------------------------------------------------------------------------------------------------
vector<sql::insert_statement> sql::song_inserts_from_directory(const string& directory_name)
// ----------------------------------------------------------------------------------------------------------------------
{
    fs::path dir{ directory_name };
    if (!fs::exists(dir)) {
        return {};
    }

    std::vector<insert_statement> data;

    for (auto entry : fs::recursive_directory_iterator{ dir }) {
        const fs::path path{ entry.path() };
        const fs::file_status status{ fs::status(path) };

        // ignore folders for albums or collections of songs
        if (fs::is_directory(status)) {
            continue;
        }

        const std::string extension = { path.extension().string() };
        // ignore files without extension completly
        if (extension.length() == 0) {
            continue;
        }

        // check if the extension is a supported music format
        if (extension != ".mp3" && extension != ".wma") {
            continue;
        }

        // file should be a song by now
        const std::string song_name = path.stem().string();

        //const std::string statement = "INSERT INTO Songs VALUES('" + song_name + "', '" + path.string() + "', '" + extension + "');";
        //data.push_back(statement);
        data.push_back(song_name);
    }

    return data;
}

// ----------------------------------------------------------------------------------------------------------------------
string sql::file_to_string(const string& sql_file_name)
// ----------------------------------------------------------------------------------------------------------------------
{
    string line = {};
    ifstream sql_file_stream;
    string sql_text = {};
    sql_file_stream.open(sql_file_name, std::ios::binary);
    while (getline(sql_file_stream, line)) {
        sql_text += line;
    }
    replace(std::begin(sql_text), std::end(sql_text), '\t', ' ');
    replace(std::begin(sql_text), std::end(sql_text), '\r', ' ');
    return sql_text;
}

// ----------------------------------------------------------------------------------------------------------------------
vector<string> sql::table_names(const sql_wrapper* db)
// ----------------------------------------------------------------------------------------------------------------------
{
    auto [result, error] = db->execute_cmd("SELECT name FROM sqlite_master WHERE type='table' ORDER BY name;");
    if (error) {
        // LOGGING!
        return {};
    }

    vector<string> names{};
    for (auto row : result) {
        names.push_back(row.front());
    }
    return names;
}

// ----------------------------------------------------------------------------------------------------------------------
unique_ptr<sql_wrapper> sql::copy_database(const sql_wrapper* db_to_copy, const string& new_name)
// ----------------------------------------------------------------------------------------------------------------------
{
    /*scope the streams*/ {
        ifstream src(db_to_copy->db_name(), std::ios::binary);
        ofstream dst(new_name,              std::ios::binary);
        dst << src.rdbuf();
    }
    return make_unique<sql_wrapper>(new_name);
}