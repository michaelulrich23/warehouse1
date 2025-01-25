#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"

int validParameterCheck(){
    for (int i = 0; i < strlen(optarg); i++) {
        if(isdigit(optarg[i]) || optarg[i] == '.'){

        }
        else
            return 4;
    }
    return 0;
}

int WH_nameFilter(char warehouse_name[], WAREHOUSE *new_WH, int * db_size_counter) {
    int j=0;
    for (int i = 0; i < DB_NUM; i++) {
        if(strncmp(db[i].name, warehouse_name, strlen(warehouse_name)) == 0){
            new_WH[j] = db[i];
            j++;
            (*db_size_counter)++;
        }
    }
    return j;
}

int WH_itemFilter(char item_name[], WAREHOUSE *new_WH, int * db_size_counter){
    int j=0;
    for (int i = 0; i < DB_NUM; i++) {
        for (int k = 0; k < MAX_ITEMS; ++k) {
            if(strncmp(db[i].items[k].name, item_name, strlen(item_name)) == 0){
                new_WH[j] = db[i];
                j++;
                (*db_size_counter)++;
            }
        }
    }
    return j;
}

void WH_gpsFilter(GPS warehouse_gps, WAREHOUSE *new_WH, int * db_size_counter){
    double tmp1, tmp2=20038; //najvacsia mozna vzdialenost dvoch bodov od seba na zemi
    int shortest_dist_index;
    for (int i = 0; i < DB_NUM; i++) {
        tmp1 = distance(warehouse_gps, db[i].gps);
        if(tmp1 < tmp2){
            shortest_dist_index = i;
            tmp2 = tmp1;
        }
    }
    new_WH[0] = db[shortest_dist_index];
    (*db_size_counter)++;
}

int item_or_price_itemFilter(bool itemF, bool priceF, char item_name_itemF[], int max_price, WAREHOUSE *new_WH, WAREHOUSE *new_item, const int * db_size_counter, int * item_size_counter){
    int WH=0, ITEM=0;
    for (int i = 0; i < (*db_size_counter); i++) {
        for (int j = 0; j < new_WH[i].n; j++) {
            if ( (strncmp(item_name_itemF, new_WH[i].items[j].name, strlen(item_name_itemF)) == 0 && itemF==1)
                 || (new_WH[i].items[j].price <= max_price && priceF==1) ){

                strncpy(new_item[WH].name, new_WH[i].name, strlen(new_WH[i].name)); //WH name
                new_item[WH].gps = new_WH[i].gps; //WH gps

                strncpy(new_item[WH].items[ITEM].name, new_WH[i].items[j].name, strlen(new_WH[i].items[j].name)); //item name
                new_item[WH].items[ITEM].price = new_WH[i].items[j].price; //item price

                ITEM++;
            }
        }
        if(ITEM != 0){
            new_item[WH].n = new_WH[i].n;
            WH++;
            ITEM=0;
        }
    }
    (*item_size_counter) = WH;
    return WH;
}

int main(int argc, char *argv[]){
    bool warehouseF=0, WH_itemF=0, bLat=0, bLon=0, itemF=0, priceF=0, WH_oriented=0, filterCheck;
    GPS warehouse_gps;
    int opt, max_price;
    int WHdb_size_counter=0, itemDB_size_counter=0;
    char* optstring = ":w:i:n:e:t:p:W";
    char item_name_WHF[MAX_NAME + 1], warehouse_name[MAX_NAME + 1], item_name_itemF[MAX_NAME + 1] = {'\0'};
    while ((opt = getopt(argc, argv, optstring)) != -1) {
        switch (opt) {
            case ':':
                return 2;
                //filter 1
            case 'w':
                warehouseF = 1;
                strcpy(warehouse_name, optarg);
                break;
            case 'i':
                WH_itemF = 1;
                strcpy(item_name_WHF, optarg);
                break;
            case 'n':
                bLat = 1;
                warehouse_gps.lat = strtod(optarg, NULL);
                if(validParameterCheck()==0) {
                    if ((warehouse_gps.lat <= LAT_MIN) || (warehouse_gps.lat >= LAT_MAX))
                        return 4;
                } else return 4;
                break;
            case 'e':
                bLon = 1;
                warehouse_gps.lon = strtod(optarg, NULL);
                if(validParameterCheck()==0) {
                    if ((warehouse_gps.lon <= LON_MIN) || (warehouse_gps.lon >= LON_MAX))
                        return 4;
                } else return 4;
                break;
                //filter 2
            case 't':
                itemF = 1;
                strcpy(item_name_itemF, optarg);
                break;
            case 'p':
                priceF = 1;
                max_price = strtol(optarg, NULL, 0);
                break;
            case 'W':
                WH_oriented=1;
                break;
            default:
                return 1;
        }
    }
    if((bLon == 0 && bLat == 0) || (bLon && bLat)){}
    else return 3;
    WAREHOUSE new_WHdb[DB_NUM] = {'\0'};
    WAREHOUSE new_itemDB[DB_NUM] = {'\0'};
    //filter 1
    if(warehouseF==0 && WH_itemF==0 && bLat == 0){
        for (int i = 0; i < DB_NUM; i++) {
            new_WHdb[i] = db[i];
        }
    } else {
        if (warehouseF) { // -w
            filterCheck = WH_nameFilter(warehouse_name, new_WHdb, &WHdb_size_counter);
            if (filterCheck == 0) return 0;
        }
        if (WH_itemF) { // -i
            filterCheck = WH_itemFilter(item_name_WHF, new_WHdb, &WHdb_size_counter);
            if (filterCheck == 0) return 0;
        }
        if (bLat) { // -n -e
            WH_gpsFilter(warehouse_gps, new_WHdb, &WHdb_size_counter); //vzdy bude nejaky sklad hocijako daleko, netreba check
        }
    }
    //filter 1 done
    if(WHdb_size_counter == 0) WHdb_size_counter=DB_NUM; //ak sa nic nevyfiltrovalo, velkost db je cela
    //filter 2

    if(itemF || priceF){
        filterCheck = item_or_price_itemFilter(itemF, priceF, item_name_itemF, max_price, new_WHdb, new_itemDB, &WHdb_size_counter, &itemDB_size_counter);
        if(filterCheck == 0) return 0;
    }
    else{
        itemDB_size_counter = WHdb_size_counter;
        for (int i = 0; i < itemDB_size_counter; i++) {
            new_itemDB[i] = new_WHdb[i];
        }
    }
    //filter 2 done
    //vypis
    if(WH_oriented){ //sklado centricky -W
        int k=0, j;
        for (int i = 0; i < WHdb_size_counter; i++) {
            printf("%s %.3lf %.3lf %d :\n", new_WHdb[i].name, new_WHdb[i].gps.lat, new_WHdb[i].gps.lon, new_WHdb[i].n);
            if (strncmp(new_itemDB[k].name, new_WHdb[i].name, strlen(new_itemDB[k].name)) == 0) {
                for (j = 0; new_itemDB[k].items[j].name[0] != '\0'; j++) {
                    printf("%d. %s %d\n", j + 1, new_itemDB[k].items[j].name, new_itemDB[k].items[j].price);
                }
                k++;
            }
        }
    }
    else{ //tovaro centricky
        int inc=1, j;
        for (int i = 0; i < itemDB_size_counter; i++) {
            for (j = 0; new_itemDB[i].items[j].name[0] != '\0'; j++) {
                printf("%d. %s %d : %s %.3lf %.3lf %d\n", inc, new_itemDB[i].items[j].name, new_itemDB[i].items[j].price, new_itemDB[i].name,
                       new_itemDB[i].gps.lat, new_itemDB[i].gps.lon, new_itemDB[i].n);
                inc++;
            }
        }
    }




    return 0;
}