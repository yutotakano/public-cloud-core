#include "yagra.h"

int main(const int argc, const char *argv[]) {
  yagra_conn_t conn = yagra_init("127.0.0.1", 1234);
  yagra_print_metrics(&conn);

  yagra_define_metric(&conn, "metric1", "This is a description", YAGRA_AGGREGATION_TYPE_AVERAGE);
  yagra_print_metrics(&conn);

  yagra_batch_data_t batch = yagra_init_batch(&conn);
  yagra_print_metrics(&conn);
  yagra_observe_metric(&batch, "metric12", 10);
  yagra_print_batch( &batch);
  yagra_print_metrics(&conn);

  yagra_send_batch(&conn, &batch);
  return 0;
}
