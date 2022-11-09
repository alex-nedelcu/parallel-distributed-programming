typedef struct {
    int id;
    int quantity;
    int unit_price;
} Product;

typedef struct {
    int id;
} SaleOperation;

typedef struct {
    int id;
    int sale_operation_id;
    int product_id;
    int quantity;
} SaleOperationComponent;

typedef struct {
    int id;
    int total_price;
} Bill; // maps SaleOperation

typedef struct {
    int id;
    int bill_id;
    int product_id;
    int quantity;
} SoldItem; // maps SaleOperationComponent
