#include "request_queue.h"


RequestQueue::RequestQueue(const SearchServer& search_server)
    : inner_server(search_server)
{

}

// сделаем "обёртки" для всех методов поиска, чтобы сохранять результаты для нашей статистики
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
    // возвращаем содержимое счетчика последнего элемента
    return requests_.back().empty_count;
}

void RequestQueue::Add(QueryResult new_q) {
    requests_.push_back(new_q);                                                // добавляем новую структуру в дек
}

void RequestQueue::Pop() {
    requests_.pop_front();                                                      // удаляем переднюю структуру из дека
}

void RequestQueue::AddEmptyReq() {
    if (GetNoResultRequests() == min_in_day_) {                                 // если дек - битком
        Pop();                                                                  // выталкиваем передний
    }
    QueryResult new_q = { {}, (GetNoResultRequests() + 1) };                    // создаем новую структуру из пустого вектора доков и счетчика
    Add(new_q);                                                                 // добавляем структуру в конец дека
}
