#include "remove_duplicates.h"

#include <iostream>
void RemoveDuplicates(SearchServer& search_server) {
    SearchServer& inner = search_server;
    std::vector<int> ids_to_delete;
    std::set<std::string_view> doc_words;
    std::map<std::set<std::string_view>, int> scanning_list;                  //������� ������� ��� ���� - ��� ����, � �������� - id

    for (const int doc_id : inner) {                                     // ����������� �� ������� id_word_freqs
        for (auto [word, _] : inner.GetWordFrequencies(doc_id)) {        // ����������� �� ����������� ������� �������� ������� ��� �������� �� ����� = id
            doc_words.insert(word);
        }                                                                // ������ ����-����� ����������� ������� ����� � ��� ���� ����� ����
        if (scanning_list.count(doc_words) == 0) {                   // ��������� ���� �� ����� ���� - ��� � �������
            scanning_list[doc_words] = doc_id;                       // ���� ��� �� ������� ����� ���� � �������
        }
        else {
            ids_to_delete.emplace_back(doc_id);                      // � ���� ���� �� ������� id � ������ id ��� ��������
        }
        doc_words.clear();                                           // ���������� ��� ��� ��������� ��������

    }
    for (auto id : ids_to_delete) {
        using namespace std::literals;
        std::cout << "Found duplicate document id "s << id << std::endl;
        inner.RemoveDocument(id);
    }
}