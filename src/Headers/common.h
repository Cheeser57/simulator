#pragma once

#include <algorithm>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

// boost uuid imports
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

// boost websocket imports
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
// #include <boost/url/params_encoded_view.hpp>
#include <boost/url/parse_query.hpp>
#include <boost/url/url.hpp>

// json imports
#include <fstream>
#include <nlohmann/json.hpp>

#pragma comment(lib, "bcrypt.lib")

typedef unsigned int uint;

// uuid namespaces
namespace uuids = boost::uuids;

using uuid = uuids::uuid;

// websocket namespaces
namespace beast = boost::beast;
namespace urls = boost::urls;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;

using tcp = boost::asio::ip::tcp;

// json namespaces
using json = nlohmann::json;

inline void fail(beast::error_code ec, char const *what, bool fatal = false) {
  std::cerr << what << ": " << ec.message() << "\n";
  if (fatal)
    exit(1);
}
