# cpp-search-server
## Проект поискового сервера «SearchServer» - система поиска документов по ключевым словам.

## Принцип работы: 
Обработка запросов к поисковой системе, исключение стоп-слов и обработка минус-слов, исключаются из результатов поиска, удаление дубликатов документов, создание и обработка очереди запросов.
## Методы: 
- AddDocument   – добавляет документы в поисковый сервер
   принимает id документа, документ в виде строки, статус документа, рейтинг.  
- RemoveDocument – удаляет документ из поискового сервера. Реализована многопоточная версия метода в дополнение к однопоточной.
  принимает id документа  
- FindTopDocument – находит документы согласно запросу по ключевым словам, возможна сортировка документов по id, статусу, рейтингу. Реализована многопоточная версия метода в дополнение к однопоточной.  
- MatchDocument – находит слова в документе, соответствующие запросу к поисковому серверу. Реализована многопоточная версия метода в дополнение к однопоточной.
  принимает строку запроса, id документа.  
## Системные требования: 
компилятор С++ с поддержкой стандарта С++17 и выше.

