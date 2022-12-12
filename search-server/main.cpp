#include <algorithm>
#include <iostream>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <map>
#include <cmath>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;

string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result = 0;
    cin >> result;
    ReadLine();
    return result;
}

vector<string> SplitIntoWords(const string& text) {
    vector<string> words;
    string word;
    for (const char c : text) {
        if (c == ' ') {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        }
        else {
            word += c;
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }

    return words;
}



struct Document {
    int id;
    double relevance;
};

class SearchServer {
public:
    void SetStopWords(const string& text) {
        for (const string& word : SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }

    void AddDocument(int document_id, const string& document) {
        const vector<string> words = SplitIntoWordsNoStop(document);
        ++document_count_;
        const  double one_word_tf = 1.0 / words.size();
        for (const string& word : words) {
            word_to_document_freqs_[word][document_id] += one_word_tf;       /* map<string, map<int, double>>*/
        }
    }

    vector<Document> FindTopDocuments(const string& raw_query) const {
        const set<string> query_words = ParseQuery(raw_query);

        auto matched_documents = FindAllDocuments(query_words);

        sort(matched_documents.begin(), matched_documents.end(),
            [](const Document& lhs, const Document& rhs) {
                return lhs.relevance > rhs.relevance;
            });

        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }

private:
    map<string, map<int, double>> word_to_document_freqs_;
    set<string> stop_words_;

    int document_count_ = 0;

    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }

    vector<string> SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;
        for (const string& word : SplitIntoWords(text)) {
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }

    set<string> ParseQuery(const string& text) const {
        set<string> query_words;
        for (const string& word : SplitIntoWordsNoStop(text)) {
            query_words.insert(word);
        }
        return query_words;
    }

    vector<Document> FindAllDocuments(const set<string>& query_words) const {
        map<int, double> documents_to_relevance; // id Документа и IDF*TF 
        vector<Document> id_relevance;
        vector<int> numbers_to_delete;     //вектор с номерами id для удаления

        for (const auto& plus_word : query_words) {
            if (IfMinusWordsFromQuery(plus_word)) {
                continue;
            }
            if (word_to_document_freqs_.count(plus_word) == 0) {
                continue;
            }
            int documents_with_word_count = word_to_document_freqs_.at(plus_word).size();
            double word_idf = log((document_count_ * 1.0) / documents_with_word_count);
            for (const auto& [doc_id, word_tf] : word_to_document_freqs_.at(plus_word)) {


                double idf_tf = word_idf * word_tf;     // IDF * TF
                documents_to_relevance[doc_id] += idf_tf;
            }
        }
        for (auto minus_word : query_words) {
            if (!IfMinusWordsFromQuery(minus_word)) {
                continue;
            }
            if (word_to_document_freqs_.count(minus_word.erase(0, 1)) == 0) {
                continue;
            }
            for (const auto& [doc_id, word_tf] : word_to_document_freqs_.at(minus_word)) {
                numbers_to_delete.push_back(doc_id);   //вектор с номерами id для удаления
            }
        }

        for (const int& number : numbers_to_delete) {
            documents_to_relevance.erase(number);
        }

        for (const auto& [document_id, rel] : documents_to_relevance) {
            id_relevance.push_back({ document_id, rel });
        }
        return id_relevance;
    }

    bool IfMinusWordsFromQuery(const string& query_word) const {
        if (query_word[0] == '-') {
            return true;
        } return false;
    }

};

SearchServer CreateSearchServer() {
    SearchServer search_server;
    search_server.SetStopWords(ReadLine());

    const int document_count = ReadLineWithNumber();
    for (int document_id = 0; document_id < document_count; ++document_id) {
        search_server.AddDocument(document_id, ReadLine());
    }

    return search_server;
}

int main() {
    const SearchServer search_server = CreateSearchServer();

    const string query = ReadLine();
    for (const auto& [document_id, relevance] : search_server.FindTopDocuments(query)) {
        cout << "{ document_id = "s << document_id << ", "
            << "relevance = "s << relevance << " }"s << endl;
    }
}