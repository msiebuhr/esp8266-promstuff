#ifndef PROM_CLIENT
#define PROM_CLIENT

/*
// Prom counter
typedef struct  {
  char* name;
  char* help;
  char* tags;
  double val;
} prom_counter;

prom_counter* prom_counter_new(char* name, char* help, char* tags) {
  prom_counter* c = (prom_counter*) malloc(sizeof(prom_counter));
  c->name = name;
  c->help = help;
  c->tags = tags;
  c->val = 0;
  return c;
}

void prom_counter_inc(prom_counter* pc) {
  pc->val += 1;
}

void prom_counter_write_to_serial(prom_counter* pc) {
  Serial.print("# HELP ");
  Serial.print(pc->name);
  Serial.print(" ");
  Serial.print(pc->help);
  Serial.print("\n# TYPE counter\n");
  Serial.print(pc->name);
  Serial.print("{");
  Serial.print(pc->tags);
  Serial.print("} ");
  Serial.print(pc->val);
  Serial.print("\n\n");
};

int prom_counter_printf(prom_counter* pc, char* in) {
  return sprintf(in, "# HELP %s %s\n# TYPE %s counter\n%s{%s} %f\n\n", pc->name, pc->help, pc->name, pc->name, pc->tags, pc->val);
};

// Prom gauge
typedef struct  {
  char* name;
  char* help;
  char* tags;
  double val;
} prom_gauge;

prom_gauge* prom_gauge_new(char* name, char* help, char* tags) {
  prom_gauge* c = (prom_gauge*) malloc(sizeof(prom_gauge));
  c->name = name;
  c->help = help;
  c->tags = tags;
  c->val = 0;
  return c;
};

void prom_gauge_set(prom_counter* pg, double val) {
  pg->val = val;
};

int prom_gauge_printf(prom_gauge* pc, char* in) {
  return sprintf(in, "# HELP %s %s\n# TYPE %s gauge\n%s{%s} %f\n\n", pc->name, pc->help, pc->name, pc->name, pc->tags, pc->val);
};

// Prom histogram
// User should be able to set linear and exponential. I just set a max and use log10(bucketSize) as size...
typedef struct {
  char* name;
  char* help;
  char* tags;
  uint buckets;
  double[] vals;
} prom_histogram;

prom_histogram* new_prom_histogram(char* name, char* help, char* tags, uint max_size) {
    int ordersOfMagnitude = (int) floor(log10(max_size));
    // Zero-bucket, 10 for each order or magnitude and one for infinite
    int buckets = 1 + ordersOfMagnitude * 10 + 1;

    

}

// Prom summary
*/
#endif
