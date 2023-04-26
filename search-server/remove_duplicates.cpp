#include "remove_duplicates.h"

#include <iostream>
void RemoveDuplicates(SearchServer& search_server) {
    SearchServer& inner = search_server;
    std::vector<int> ids_to_delete;
    std::set<std::string_view> doc_words;
    std::map<std::set<std::string_view>, int> scanning_list;                  //создаем словарь где ключ - сет слов, а значение - id

    for (const int doc_id : inner) {                                     // итерируемся по словарю id_word_freqs
        for (auto [word, _] : inner.GetWordFrequencies(doc_id)) {        // итерируемся по внутреннему словарю значений получив его функцией по ключу = id
            doc_words.insert(word);
        }                                                                // каждое ключ-слово внутреннего словаря пишем в сет слов этого дока
        if (scanning_list.count(doc_words) == 0) {                   // проверяем есть ли такой ключ - сет в словаре
            scanning_list[doc_words] = doc_id;                       // если нет то создаем такую пару в словаре
        }
        else {
            ids_to_delete.emplace_back(doc_id);                      // а если есть то толкаем id в вектор id для удаления
        }
        doc_words.clear();                                           // опустошаем сет для следующей итерации

    }
    for (auto id : ids_to_delete) {
        using namespace std::literals;
        std::cout << "Found duplicate document id "s << id << std::endl;
        inner.RemoveDocument(id);
    }
}