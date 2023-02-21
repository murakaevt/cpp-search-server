#pragma once
#include "search_server.h"
#include <deque>
#include <vector>

class RequestQueue {
public:
    explicit RequestQueue(const SearchServer& search_server);

    // ������� "������" ��� ���� ������� ������, ����� ��������� ���������� ��� ����� ����������
    template <typename DocumentPredicate>
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate);
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentStatus status);
    std::vector<Document> AddFindRequest(const std::string& raw_query);
    int GetNoResultRequests() const;
private:
    struct QueryResult {
        std::vector<Document> found_docs;
        int empty_count;
    };

    std::deque<QueryResult> requests_;
    const static int min_in_day_ = 1440;

    SearchServer inner_server;

    void Add(QueryResult new_q);
    // ��������� ����� ��������� � ���
    void Pop();
    // ������� �������� ��������� �� ����
    void AddEmptyReq();
    // ���� ��� - ������
                                        // ����������� ��������
// ������� ����� ��������� �� ������� ������� ����� � ��������
                                 // ��������� ��������� � ����� ����
};
