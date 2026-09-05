## Autore:    Giuseppe Nozza
## Matricola: 689223
## Email:     g.nozza@studenti.unipi.it

# ClickHouse Ingestion and Behavioural Analysis (CHIBA)

## 1. Descrizione del progetto

### 1.1 Obiettivi
Lo scopo di questo tool di telemetria è quello di analizzare il comportamento di flussi di rete in tempo reale
e da file PCAP ai fini del rilevamento di DDoS, esfiltrazione di dati, network scan, port scan rapido e lento.


### 1.2 Dalla cattura con la sonda all'esportazione binaria
```
+--------------------------------------------+
| BOOT: chiba [-h] [-r <path>] [-i <device>] |
| * Caricamento a runtime di chiba.conf      |
| * fork ed exec                             |
|--------------+-------------------------+---+
               |                         |
               |                         |
               |                         |
               |                         |
               v                         |
+-----------------------------+          |
|  PROCESSO FIGLIO: softflowd |          |
| (Sonda cattura live / PCAP) |          |
+--------------+--------------+          |
               |                         |
               | Datagrammi UDP          | 
               v                         v
+--------------+-------------------------+----------------------------------------------+
|                             PROCESSO PADRE: CHIBA Daemon                              |
|                                                                                       |
|  +---------------------+     +---------------------+     +---------------------+      |
|  |  THREAD AUSILIARIO  |     | SHARED RING BUFFER  |     |     MAIN THREAD     |      |
|  |    (Collettore)     |     | (Buffer Circolare)  |     |    (Esportatore)    |      |
|  |                     |     |                     |     |                     |      |
|  | * Polling non       |     | * Coda FIFO         |     | * Trigger           |      |
|  |   bloccante         |     | * Sincronizzazione  |     |   temporale         |      |
|  | * Parsing record    |     |   con una lock      |     | * Trigger           |      |
|  |   NF9               |     |                     |     |   volumetrico       |      |
|  +----------+----------+     +------+-------+------+     +--------+-------+----+      |
|             |                       ^       |                     ^       |           |
|             +-----------------------+       +---------------------+       |           |
|                                                                           |           |
+---------------------------------------------------------------------------+-----------+
                                                                            |
                                                                 Trasferimento in massa
                                                                 HTTP POST (RowBinary)
                                                                            |
                                                                            v
+---------------------------------------------------------------------------------------+
|                                   CLICKHOUSE SERVER                                   |
|                                     (Ingestione)                                      |
+---------------------------------------------------------------------------------------+
```
### 1.2.1 Gestione della configurazione e parametri CLI. 
Le variabili d'ambiente CH_HOST, 

### 1.2.2 Gestione di softflowd

### 1.2.3 Parsing dei record NetFlow v9

### 1.2.4 Pattern produttore-consumatore, double buffering

### 1.2.5 Streaming HTTP

### 1.3 Schema e pipeline dati in ClickHouse
```
+--------------------------------------------------------------------------------------------------+
|                              TABELLA PRINCIPALE: chiba.ingest_flows                              |
+------------------------------------------------+-------------------------------------------------+
                                                 |
             +-----------------------------------+-----------------------+  Trigger Materialized Views
             |                                   |                       |  (Calcolo stati incrementali)
             v                                   v                       v
+------------+------------+             +--------+---------+     +--------+--------+
|   chiba.host_src_agg    |             |chiba.host_dst_agg|     |chiba.service_agg|
|                         |             |                  |     |                 |
| * Aggregati host        |             | * Aggregati      |     | * Aggregati per |
|   sorgente              |             |   target         |     |   servizio (L4) |
| * Unicita' e volumi     |             | * Volume di      |     | * Ripartizione  |
|   in uscita             |             |   traffico in    |     |   del carico    |
+------------+------------+             +--------+---------+     +--------+--------+
             |                                   |                       |
     +-------+-------+                           |                       |  Risoluzione stati -Merge
     |               |                           |                       |
     v               v                           v                       v
+----+------+ +------+-----+             +-------+-------+       +-------+-------+
|chiba.host_| |slow_       |             |  chiba.host_  |       | chiba.service |
|src_view   | |scanners_   |             |  dst_view     |       | _view         |
|           | |view        |             |               |       |               |
|* Scan     | |* Scansioni |             | * DDoS e      |       | * Traffico L4 |
|  verticali|   stealth a  |             |   flood       |       | * Porte       |
|  e orizz. |   bassa freq.|             |   volumetrici |       |   maggiormente|
|* Egress   | * Finestra   |             | * Tassi PPS   |       |   sollecitate |
|  anomalo  |   estesa 24h |             |   e BPS       |       |               |
+----+------+ +------+-----+             +-------+-------+       +-------+-------+
     |               |                           |                       |
     +---------------+---------------------------+-----------------------+
                                 |
                                 | Query analitiche
                                 v
+--------------------------------------------------------------------------------------------------+
|                                        DASHBOARD GRAFANA                                         |
|                               (Telemetria e Rilevamento Anomalie)                                |
+--------------------------------------------------------------------------------------------------+
```
### 1.3.1 Schema della tabella base 

### 1.3.2 Aggregazione dei dati e Materialized Views

### 1.3.3 Logica delle Views

### 1.4 Monitoraggio e alerting

### 1.4.1 Pannelli di traffico e telemetria

### 1.4.2 Regole di rilevamento, soglie, severità

## 2. Prerequisiti e istruzioni per l'esecuzione

## 3. Testing

## 4. Conclusioni e sviluppi futuri

## 5. Riferimenti e bibliografia


