# Поисковый сервер потерянных животных
Один из проектов, который был реализован во время обучения на Яндекс Практикуме.
Проект представляет собой модель поисковой системы. Осуществляет поиск документов по ключевым словам и поддерживает:

1. Ранжирование результатов по статистической мере TF-IDF
2. Функциональность минус-слов и стоп-слов
3. Обработка очереди запросов
4. Многопоточный режим.

# Описание
## Методы классы
В основе поискового сервера используется класс SearchServer. 
  - Конструктор данного класса принимает в качестве параметров набор стоп-слов. Стоп-слово в запросе не учитывается при поиске.
  - Добавление документов в поисковую систему. `void AddDocument (int document_id, string_view document, статус DocumentStatus, const vector<int> &ratings)`; 

Возможные статусы документов: `ACTUAL`, `IRRELEVANT`, `BANNED`, `REMOVED`.

  - Поиск документов на поисковом сервере и ранжирование по TF-IDF
    Есть 6 способов вызова функции 3 многопоточных (ExecutionPolicy) и 3 однопоточных: <br/>
    `FindTopDocuments (ExecutionPolicy,query)`<br/>
    `FindTopDocuments (ExecutionPolicy,query,DocumentStatus)` <br/>
    `FindTopDocuments (ExecutionPolicy,query,DocumentPredicate)`<br/>
    `FindTopDocuments (query)`<br/>
    `FindTopDocuments (query,DocumentStatus)`<br/>
    `FindTopDocuments (query,DocumentPredicate)`<br/>
 - `GetDocumentCount()` - возвращает количество документов на сервере поиска
 - `tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(raw_query, document_id)` - Возвращает: Первый объект — это вектор слов запроса, которые были найдены в документе `document_id`, а второй объект это статус документа
 - `map<string, double> GetWordFrequencies(document_id)` - Метод получения частот слов по идентификатору документа
 - Удаление документов из поискового сервера:<br/>
    `RemoveDocument(document_id)`<br/>
    `RemoveDocument(ExecutionPolicy,document_id)`<br/>
    `RemoveDocument(ExecutionPolicy, document_id)`<br/>

