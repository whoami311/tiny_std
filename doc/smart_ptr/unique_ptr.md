```plantuml
@startuml

class __uniq_ptr_impl
class __uniq_ptr_data
class unique_ptr

__uniq_ptr_impl <|-- __uniq_ptr_data
unique_ptr *-- __uniq_ptr_data

@enduml
```
