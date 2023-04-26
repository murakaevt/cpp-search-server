#include "search_server.h"
#include "log_duration.h"

SearchServer::SearchServer(const std::string& stop_words_text)
    : SearchServer(
        SplitIntoWords(stop_words_text))
{}

SearchServer::SearchServer(std::string_view stop_words_text)
    : SearchServer(SplitIntoWords(stop_words_text))
{}

void SearchServer::AddDocument(int document_id, std::string_view document, DocumentStatus status,
    const std::vector<int>& ratings) {

    if ((document_id < 0) || (documents_.count(document_id) > 0)) {
        throw std::invalid_argument("Invalid document_id");
    }
    documents_from_request.insert({ document_id, std::string(document) });

    auto words = SplitIntoWordsNoStop(documents_from_request.at(document_id)); //вектор слов к добавлению
    const double inv_word_count = 1.0 / words.size();                                            //
    for (std::string_view word : words) {
        word_to_document_freqs_[word][document_id] += inv_word_count;
        id_word_freqs_[document_id][word] += inv_word_count;
    }
    documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });
    document_ids_.push_back(document_id);
}

void SearchServer::RemoveDocument(int document_id) {

    auto pos = find(document_ids_.begin(), document_ids_.end(), document_id);  // ищем тератор где в векторе id
    document_ids_.erase(pos);                                                  // удаляем по указателю


    for (auto [key_word, _] : id_word_freqs_.at(document_id)) {                // перебираем ключи-слова внутреннего словаря

        word_to_document_freqs_.at(std::string(key_word)).erase(document_id);               // и удаляем id из внутреннего словаря 
        if (word_to_document_freqs_.at(std::string(key_word)).empty()) {                    // если ключу теперь соответствует пустое значение
            word_to_document_freqs_.erase(std::string(key_word));                           // удаляем внешний ключ
        }
    }

    id_word_freqs_.erase(document_id);                                         // удаляем по ключу
    documents_.erase(document_id);                                             // удаляем по ключу
}

void  SearchServer::RemoveDocument(std::execution::sequenced_policy seq, int document_id) {
    SearchServer::RemoveDocument(document_id);
}

void SearchServer::RemoveDocument(std::execution::parallel_policy par, int document_id) {

    std::vector<std::string_view> strings_to_del(id_word_freqs_.at(document_id).size());
    std::transform(std::execution::par, id_word_freqs_.at(document_id).begin(), id_word_freqs_.at(document_id).end(), strings_to_del.begin(),
        [](const auto& pa) {return pa.first; });
    std::for_each(std::execution::par, strings_to_del.begin(), strings_to_del.end(),
        [&](const std::string_view string_to_del) {word_to_document_freqs_.at(string_to_del).erase(document_id); });

    id_word_freqs_.erase(document_id);                                         // удаляем по ключу
    documents_.erase(document_id);                                             // удаляем по ключу
}

std::vector<Document> SearchServer::FindTopDocuments(std::string_view raw_query, DocumentStatus status) const {
     return SearchServer::FindTopDocuments(
            raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
                return document_status == status;
            });
    }

std::vector<Document> SearchServer::FindTopDocuments(std::execution::sequenced_policy seq, std::string_view raw_query, DocumentStatus status) const {
    return SearchServer::FindTopDocuments(
        raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
            return document_status == status;
        });
}

std::vector<Document> SearchServer::FindTopDocuments(std::execution::parallel_policy par, std::string_view raw_query, DocumentStatus status) const {
    return SearchServer::FindTopDocuments(par,
        raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
            return document_status == status;
        });
}

    std::vector<Document> SearchServer::FindTopDocuments(std::string_view raw_query) const {
        return SearchServer::FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
    }

    std::vector<Document> SearchServer::FindTopDocuments(std::execution::sequenced_policy seq, std::string_view raw_query) const {
        return SearchServer::FindTopDocuments(seq, raw_query, DocumentStatus::ACTUAL);
    }

    std::vector<Document> SearchServer::FindTopDocuments(std::execution::parallel_policy par, std::string_view raw_query) const {
        return SearchServer::FindTopDocuments(par, raw_query, DocumentStatus::ACTUAL);
    }

int SearchServer::GetDocumentCount() const {
    return static_cast<int>(documents_.size());
}

const std::map<std::string_view, double>& SearchServer::GetWordFrequencies(int document_id) const {
    static const std::map<std::string_view, double> res;
    if (id_word_freqs_.find(document_id) == id_word_freqs_.end()) {
        return res;
    }
    return id_word_freqs_.at(document_id);
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(std::string_view raw_query,
    int document_id) const {
    if (!IsValidWord(raw_query)) {
        using namespace std::literals;
        throw std::invalid_argument("Word "s + std::string{ raw_query } + " is invalid"s);
    }
    if (std::count(document_ids_.begin(), document_ids_.end(), document_id) == 0) {
        throw std::out_of_range("out_of_range");
    }

    auto query = ParseQuery(raw_query);
    std::sort(query.plus_words.begin(), query.plus_words.end());
    query.plus_words.erase(std::unique(query.plus_words.begin(), query.plus_words.end()), query.plus_words.end());

    std::sort(query.minus_words.begin(), query.minus_words.end());
    query.minus_words.erase(std::unique(query.minus_words.begin(), query.minus_words.end()),
        query.minus_words.end());

    if (std::any_of(query.minus_words.begin(), query.minus_words.end(),
        [&](std::string_view minus_word) {return word_to_document_freqs_.at(minus_word).count(document_id); })) {
        return { std::vector<std::basic_string_view<char>>{}, documents_.at(document_id).status };
    }
    std::vector<std::string_view> matched_words(query.plus_words.size());
    matched_words.resize(query.plus_words.size());
    auto it = std::copy_if(query.plus_words.begin(), query.plus_words.end(), matched_words.begin(),
        [&](std::string_view plus_word) {return word_to_document_freqs_.at(plus_word).count(document_id); });
    // matched_words.resize(std::distance(matched_words.begin(), it));
    std::sort(matched_words.begin(), it);
    matched_words.erase(std::unique(matched_words.begin(), it),
        matched_words.end());

    return { matched_words, documents_.at(document_id).status };
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(std::execution::sequenced_policy seq,
    std::string_view raw_query, int document_id) const {
    return SearchServer::MatchDocument(raw_query, document_id);
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(std::execution::parallel_policy par,
    std::string_view raw_query, int document_id) const {
    // Check if the document_id is valid
    if ((document_id < 0) || (documents_.count(document_id) <= 0)) {
        throw std::out_of_range("Invalid document_id");
    }

    auto query = ParseQuery(raw_query);

    if (std::any_of(std::execution::par, query.minus_words.begin(), query.minus_words.end(),
        [&](std::string_view minus_word) {return word_to_document_freqs_.at(minus_word).count(document_id); })) {
        return { std::vector<std::basic_string_view<char>>{}, documents_.at(document_id).status };
    }
    std::vector<std::string_view> matched_words = {};
    matched_words.resize(query.plus_words.size());
    auto it = std::copy_if(std::execution::par, query.plus_words.begin(), query.plus_words.end(), matched_words.begin(),
        [&](std::string_view plus_word) {return word_to_document_freqs_.at(plus_word).count(document_id); });
    // matched_words.resize(std::distance(matched_words.begin(), it));
    std::sort(std::execution::par, matched_words.begin(), it);
    matched_words.erase(std::unique(std::execution::par, matched_words.begin(), it),
        matched_words.end());

    return { matched_words, documents_.at(document_id).status };
}

bool SearchServer::IsStopWord(std::string_view word) const {
    return stop_words_.count(word) > 0;
}

bool SearchServer::IsValidWord(std::string_view word) {
    // A valid word must not contain special characters
    return std::none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
        });
}

std::vector<std::string_view> SearchServer::SplitIntoWordsNoStop(std::string_view text) const {
    using namespace std::literals;
    std::vector<std::string_view> words;
    for (std::string_view word : SplitIntoWords(text)) {
        if (!IsValidWord(word)) {
            throw std::invalid_argument("Word "s + std::string(word) + " is invalid"s);
        }
        if (!IsStopWord(word)) {
            words.push_back(word);
        }
    }
    return words;
}

int SearchServer::ComputeAverageRating(const std::vector<int>& ratings) {
    if (ratings.empty()) {
        return 0;
    }
    int rating_sum = accumulate(ratings.begin(), ratings.end(), 0);

    return rating_sum / static_cast<int>(ratings.size());
}

SearchServer::QueryWord SearchServer::ParseQueryWord(std::string_view text) const {
    using namespace std::literals;
    if (text.empty()) {
        throw std::invalid_argument("Query word is empty"s);
    }
    std::string_view word = text;
    bool is_minus = false;
    if (word[0] == '-') {
        is_minus = true;
        word = word.substr(1);
    }
    if (word.empty() || word[0] == '-' || !IsValidWord(word)) {
        throw std::invalid_argument("Query word "s + std::string(text) + " is invalid");
    }

    return { word, is_minus, IsStopWord(word) };
}

SearchServer::Query SearchServer::ParseQuery(std::string_view text) const {
    Query result;
    for (std::string_view word : SplitIntoWords(text)) {
        const auto query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                result.minus_words.push_back(query_word.data);
            }
            else {
                result.plus_words.push_back(query_word.data);
            }
        }
    }
    return result;
}

// Existence required
double SearchServer::ComputeWordInverseDocumentFreq(std::string_view word) const {
    return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}


std::vector<int>::const_iterator SearchServer::begin() {
    return document_ids_.begin();
}


std::vector<int>::const_iterator SearchServer::end() {
    return document_ids_.end();
}