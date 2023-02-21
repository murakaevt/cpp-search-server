#include "request_queue.h"


RequestQueue::RequestQueue(const SearchServer& search_server)
    : inner_server(search_server)
{

}

// ������� "������" ��� ���� ������� ������, ����� ��������� ���������� ��� ����� ����������
template <typename DocumentPredicate>
std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate) {
    std::vector<Document> result = inner_server.FindTopDocuments(raw_query, document_predicate);
    return result;
}
std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentStatus status) {
    std::vector<Document> result = inner_server.FindTopDocuments(raw_query, status);
    return result;
}
std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query) {
    std::vector<Document> result = inner_server.FindTopDocuments(raw_query);
    return result;
}
int RequestQueue::GetNoResultRequests() const {
    // ���������� ���������� �������� ���������� ��������
    return requests_.back().empty_count;
}

void RequestQueue::Add(QueryResult new_q) {
    requests_.push_back(new_q);                                                // ��������� ����� ��������� � ���
}

void RequestQueue::Pop() {
    requests_.pop_front();                                                      // ������� �������� ��������� �� ����
}

void RequestQueue::AddEmptyReq() {
    if (GetNoResultRequests() == min_in_day_) {                                 // ���� ��� - ������
        Pop();                                                                  // ����������� ��������
    }
    QueryResult new_q = { {}, (GetNoResultRequests() + 1) };                    // ������� ����� ��������� �� ������� ������� ����� � ��������
    Add(new_q);                                                                 // ��������� ��������� � ����� ����
}
