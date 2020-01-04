#include <iostream>
#include <stdio.h>
#include <string>
#include <fstream>
#include <sstream>
#include <math.h>
#include <iomanip>
#include <cmath>
#include <stdlib.h>

using namespace std;

string memory_request, file, mem_request, info;
stringstream ss;
double blockoffset_bits, index_bits_L1, tag_bits_L1, index_bits_L2, tag_bits_L2, sectoroffset_bits_L2, addresstags_bits_L2; 
int readrequest, writerequest, L1_ASSOC, L2_ASSOC, L2_DATA_BLOCKS, L2_ADDRESS_TAGS; 
bool flag;
long memory_address;

class Tagstore{

public:
long tag;
int counter;  
char dirtybit = 'N';
char validbit = 'I';
int selectionbit;
};

class Data{
public:  
char dirtybit = 'N';
char validbit = 'I';
int selectionbit;
};

class Cache {
private : 
int i, access, read_hit, write_hit, L_ASSOC;
long blockaddress, blockoffset, index, tag, evict_tag, P, blockindex, blocktag, N, tag_store[5000][5000];
char dirty_store[5000][5000];
public :
int write_miss, read_miss, readaccess, writeaccess, writeback, sectormiss, blockinvalid;
long index_L1, tag_L1, index_L2, tag_L2;
void readbasic(int);
void writebasic(int);
void readcoupled(int);
void writecoupled(int);
int LRU_counter(int);
void displaybasic(long, int, string);
void displaysector(long, int, string);
void addressdecoder1(string);
void addressdecoder2(string);
Cache *nextLevel;
Tagstore tagstore[5000][5000];
Data dataarray[5000][5000];
};
Cache cache_L1, cache_L2;

/*--------------------------------------Function to check if the blocksize is power of two------------------------------------*/
bool isPoweroftwo(int size){
if (size == 0) {
cout <<"blocksize is not valid";
return 0;  
}

while (size != 1){  
if ((size)%2!= 0){
cout <<"blocksize is not valid";
return 0; 
} 
size = size/2;
}   
return 1;  
}
/*-----------------------------------------------Function to read trace file--------------------------------------------*/
void getaddress(string file) {
ifstream readfile (file);

if(!readfile) {
cout << "Cannot open file.\n";
}

while (getline(readfile, memory_request)) {
cache_L1.addressdecoder1(memory_request);
//break;
}

readfile.close();
}
/*------------------------------------------------Function to decode the address-------------------------------------------*/
void Cache::addressdecoder1(string memory_request){

if (memory_request[0]== 'r'){
readrequest = 1;
}
else if(memory_request[0]== 'w'){
writerequest = 1;
}

for (int i = 0; i<11; ++i){
memory_request[i] = memory_request[i+2];
if (memory_request[i]=='\n'){
break;
}
}

memory_address = stol(memory_request, nullptr, 16);
blockoffset    = fmod(memory_address, pow(2, blockoffset_bits));
blockaddress   = memory_address/pow(2, blockoffset_bits);
index          = fmod(blockaddress, pow(2, index_bits_L1));
tag            = blockaddress/pow(2, index_bits_L1);

if(readrequest){
readrequest = 0;
if(L1_ASSOC==1){                     /*((L2_ADDRESS_TAGS == 1) && (L2_DATA_BLOCKS == 1)) yestchange*/
readbasic(L1_ASSOC);
}
else{
readcoupled(L1_ASSOC);    
}
}
else{
writerequest = 0;
if(L1_ASSOC==1){
writebasic(L1_ASSOC);
}
else{
writecoupled(L1_ASSOC);    
}
}
}

void Cache::addressdecoder2(string memory_request){

if (memory_request[0]== 'r'){
readrequest = 1;
}
else if(memory_request[0]== 'w'){
writerequest = 1;
}

for (int i = 0; i<11; ++i){
memory_request[i] = memory_request[i+2];
if (memory_request[i]=='\n'){
break;
}
}

memory_address = stol(memory_request, nullptr, 16);
blockoffset  = fmod(memory_address, pow(2, blockoffset_bits));
blockaddress = memory_address/pow(2, blockoffset_bits);
P            = fmod(blockaddress, pow(2, sectoroffset_bits_L2));
blockindex   = blockaddress/pow(2, sectoroffset_bits_L2);
index        = fmod(blockindex, pow(2, index_bits_L2));
blocktag     = blockindex/pow(2, index_bits_L2);
N            = fmod(blocktag, pow(2, addresstags_bits_L2));
tag          = blocktag/pow(2, addresstags_bits_L2);

if(readrequest){
readrequest = 0;
if((L2_ADDRESS_TAGS == 1) && (L2_DATA_BLOCKS == 1)){
readbasic(L2_ASSOC);    
}
else{
readcoupled(N*L2_ASSOC);
}
}
else{
writerequest = 0;
if((L2_ADDRESS_TAGS == 1) && (L2_DATA_BLOCKS == 1)){
writebasic(L2_ASSOC);    
}
else{
writecoupled(N*L2_ASSOC);
}
}
}

void Cache::readcoupled(int L_ASSOC){

access++;
readaccess++;

/*--------------------------------------------------L1-read-------------------------------------------------------------------------*/
if(nextLevel != nullptr){
/*------------------------------------read-hit-case---------------------------------------------------------------------------------*/
for(i=0; i<L_ASSOC; ++i){
if ((tag == tagstore[index][i].tag) && (tagstore[index][i].validbit == 'V')){          
read_hit++;
LRU_counter(L_ASSOC);
}
}  
/*------------------------------------read-miss-case--------------------------------------------------------------------------------*/ 
if((i=(L_ASSOC-1)) && (read_hit == 0)){                                                

read_miss++;
int k = LRU_counter(L_ASSOC);                                                           
/*------------------------------------if-block-dirty--------------------------------------------------------------------------------*/
if(tagstore[index][k].dirtybit == 'D'){                                                                              
tagstore[index][k].dirtybit = 'N';
writeback++;
evict_tag = tagstore[index][k].tag; 
blockaddress = (evict_tag * pow(2, index_bits_L1)) + index;
memory_address = (pow(2, blockoffset_bits)) * blockaddress;
stringstream ss;
ss << 'w';
ss << " ";
ss << hex << memory_address;
nextLevel->addressdecoder2(ss.str());                                                  
}
/*------------------------------------read-from-L2----------------------------------------------------------------------------------*/
if(memory_request[0]== 'w'){
memory_request[0] = 'r';
}
nextLevel->addressdecoder2(memory_request);

tagstore[index][k].tag = tag;
tagstore[index][k].validbit = 'V';
}
}
/*--------------------------------------------------L2-read---------------------------------------------------------------------------*/
else{
/*------------------------------------read-hit-case-----------------------------------------------------------------------------------*/    
if ((tag == tagstore[index][N].tag) && (dataarray[index][P].validbit == 'V') && (dataarray[index][P].selectionbit == N)){
read_hit++;    
}
/*------------------------------------read-miss-case----------------------------------------------------------------------------------*/
else{
read_miss++;

if(dataarray[index][P].validbit == 'I'){
for(i=0; i<L2_DATA_BLOCKS; i++){
if(dataarray[index][i].validbit == 'I'){
blockinvalid++;    
}
}
if(blockinvalid == L2_DATA_BLOCKS){
sectormiss++; 
} 
blockinvalid = 0;      
}

if((tag == tagstore[index][N].tag) && (dataarray[index][P].selectionbit == N) && (dataarray[index][P].validbit == 'I')){
dataarray[index][P].validbit = 'V';
}

if((tag != tagstore[index][N].tag) && (dataarray[index][P].selectionbit == N)){    
for(i=0; i<L2_DATA_BLOCKS; i++){
if(dataarray[index][i].selectionbit == N){
if(dataarray[index][i].dirtybit == 'D'){
writeback++;
dataarray[index][i].dirtybit = 'N';
}
dataarray[index][i].validbit = 'I';
dataarray[index][i].selectionbit = 0; 
}
}
tagstore[index][N].tag = tag;
dataarray[index][P].validbit = 'V';
dataarray[index][P].selectionbit = N;     
}

if((tag != tagstore[index][N].tag) && (dataarray[index][P].selectionbit != N)){
for(i=0; i<L2_DATA_BLOCKS; i++){
if(dataarray[index][i].selectionbit == N){
if(dataarray[index][i].dirtybit == 'D'){
writeback++;
dataarray[index][i].dirtybit = 'N';
}
dataarray[index][i].validbit = 'I';
dataarray[index][i].selectionbit = 0; 
}
}
if(dataarray[index][P].dirtybit == 'D'){
writeback++;
dataarray[index][P].dirtybit = 'N';
}
tagstore[index][N].tag = tag;
dataarray[index][P].validbit = 'V';
dataarray[index][P].selectionbit = N;
}

if((tag == tagstore[index][N].tag) && (dataarray[index][P].selectionbit != N)){
if(dataarray[index][P].dirtybit == 'D'){
writeback++;
dataarray[index][P].dirtybit = 'N';
}
dataarray[index][P].validbit = 'V';
dataarray[index][P].selectionbit = N;
}
}
}
read_hit = 0;
}

void Cache::readbasic(int L_ASSOC){

access++;
readaccess++;

for(i=0; i<L_ASSOC; ++i){
if ((tag == tagstore[index][i].tag) && (tagstore[index][i].validbit == 'V')){
read_hit++;
LRU_counter(L_ASSOC);
break;    /*optional*/
}
}

if((i=(L_ASSOC)) && (read_hit == 0)){                         /*yestchange*/

read_miss++;
int k = LRU_counter(L_ASSOC);
if(tagstore[index][k].dirtybit == 'D'){

tagstore[index][k].dirtybit = 'N';
writeback++;

if(nextLevel != nullptr){

evict_tag = tagstore[index][k].tag; 
blockaddress = (evict_tag * pow(2, index_bits_L1)) + index;
memory_address = (pow(2, blockoffset_bits)) * blockaddress;

stringstream ss;
ss << 'w';
ss << " ";
ss << hex << memory_address;
nextLevel->addressdecoder2(ss.str());

}
}

if(nextLevel != nullptr)
{
if(memory_request[0]== 'w'){
memory_request[0]= 'r';
}
nextLevel->addressdecoder2(memory_request);
}
tagstore[index][k].tag = tag;
tagstore[index][k].validbit = 'V';
}

read_hit = 0;
}

void Cache::writecoupled(int L_ASSOC){

access++;
writeaccess++;

/*--------------------------------------------------L1-write-------------------------------------------------------------------------*/
if(nextLevel != nullptr){
/*-----------------------------------write-hit-case----------------------------------------------------------------------------------*/    
for(i=0; i<L_ASSOC; ++i){
if (tag == tagstore[index][i].tag){
write_hit++;
tagstore[index][i].dirtybit = 'D';
tagstore[index][i].validbit = 'V';
LRU_counter(L_ASSOC);
}
}
/*------------------------------------write-miss-case--------------------------------------------------------------------------------*/
if((i=(L_ASSOC-1)) && (write_hit == 0)){

write_miss++; 
int k = LRU_counter(L_ASSOC); 
/*------------------------------------if-block-dirty--------------------------------------------------------------------------------*/
if(tagstore[index][k].dirtybit == 'D'){
writeback++;
evict_tag = tagstore[index][k].tag; 
blockaddress = (evict_tag * pow(2, index_bits_L1)) + index;
memory_address = (pow(2, blockoffset_bits)) * blockaddress;   
stringstream ss;
ss << 'w';
ss << " ";
ss << hex << memory_address;
nextLevel->addressdecoder2(ss.str());
}
/*------------------------------------read-from-L2----------------------------------------------------------------------------------*/
if(memory_request[0]== 'w'){
memory_request[0]= 'r';
}
nextLevel->addressdecoder2(memory_request);

tagstore[index][k].tag = tag;
tagstore[index][k].dirtybit = 'D';
tagstore[index][k].validbit = 'V';
} 
}
/*--------------------------------------------------L2-write---------------------------------------------------------------------------*/
else{
/*------------------------------------write-hit-case-----------------------------------------------------------------------------------*/     
if ((tag == tagstore[index][N].tag) && (dataarray[index][P].selectionbit == N) && (dataarray[index][P].validbit == 'V')){
write_hit++;
dataarray[index][P].dirtybit = 'D';  
} 
/*------------------------------------write-miss-case----------------------------------------------------------------------------------*/
else{
write_miss++;

if(dataarray[index][P].validbit == 'I'){
for(i=0; i<L2_DATA_BLOCKS; i++){
if(dataarray[index][i].validbit == 'I'){
blockinvalid++;    
}
}
if(blockinvalid == L2_DATA_BLOCKS){
sectormiss++;  
} 
blockinvalid = 0;     
}

if((tag == tagstore[index][N].tag) && (dataarray[index][P].selectionbit == N) && (dataarray[index][P].validbit == 'I')){
dataarray[index][P].validbit = 'V';
dataarray[index][P].dirtybit = 'D';
}

if((tag != tagstore[index][N].tag) && (dataarray[index][P].selectionbit == N)){     
for(i=0; i<L2_DATA_BLOCKS; i++){   
if(dataarray[index][i].selectionbit == N){
if(dataarray[index][i].dirtybit == 'D'){
writeback++;
dataarray[index][i].dirtybit = 'N';
}
dataarray[index][i].validbit = 'I';
dataarray[index][i].selectionbit = 0;  
}
}
tagstore[index][N].tag = tag;
dataarray[index][P].validbit = 'V';
dataarray[index][P].dirtybit = 'D';
dataarray[index][P].selectionbit = N;    
}

if((tag != tagstore[index][N].tag) && (dataarray[index][P].selectionbit != N)){
for(i=0; i<L2_DATA_BLOCKS; i++){
if(dataarray[index][i].selectionbit == N){
if(dataarray[index][i].dirtybit == 'D'){
writeback++;
dataarray[index][i].dirtybit = 'N';
}
dataarray[index][i].validbit = 'I';
dataarray[index][i].selectionbit = 0;  
}
}
if(dataarray[index][P].dirtybit == 'D'){
writeback++;
dataarray[index][P].dirtybit = 'N';
}
tagstore[index][N].tag = tag;
dataarray[index][P].validbit = 'V';
dataarray[index][P].selectionbit = N;
dataarray[index][P].dirtybit = 'D';
}

if((tag == tagstore[index][N].tag) && (dataarray[index][P].selectionbit != N)){
if(dataarray[index][P].dirtybit == 'D'){
writeback++;
dataarray[index][P].dirtybit = 'N';
}
dataarray[index][P].validbit = 'V';
dataarray[index][P].selectionbit = N;
dataarray[index][P].dirtybit = 'D';
}

}
}
write_hit = 0;
}

void Cache::writebasic(int L_ASSOC){

access++;
writeaccess++;

for(i=0; i<L_ASSOC; ++i){

if (tag == tagstore[index][i].tag){

write_hit++;
tagstore[index][i].dirtybit = 'D';
tagstore[index][i].validbit = 'V';
LRU_counter(L_ASSOC);
break;      /*optional*/

}

} 
if((i=(L_ASSOC)) && (write_hit == 0)){                         /*yestchange*/

write_miss++; 
int k = LRU_counter(L_ASSOC); 
if(tagstore[index][k].dirtybit == 'D'){

writeback++;

if(nextLevel != nullptr){
evict_tag = tagstore[index][k].tag; 
blockaddress = (evict_tag * pow(2, index_bits_L1)) + index;
memory_address = (pow(2, blockoffset_bits)) * blockaddress;   
stringstream ss;
ss << 'w';
ss << " ";
ss << hex << memory_address;
nextLevel->addressdecoder2(ss.str());
}
}

if(nextLevel != nullptr){
if(memory_request[0]== 'w'){
memory_request[0]= 'r';
}
nextLevel->addressdecoder2(memory_request);
}
tagstore[index][k].tag = tag;
tagstore[index][k].dirtybit = 'D';
tagstore[index][k].validbit = 'V';
}
write_hit = 0;
}
/*----------------------------------------------------------LRU Counter function------------------------------------------------*/
int Cache::LRU_counter(int L_ASSOC){

static int currentread_miss = 0;
static int currentwrite_miss = 0; 
if(read_hit || write_hit){
int j = i;
int temp = tagstore[index][i].counter;
for(i=0; i<L_ASSOC; ++i){
if(tagstore[index][i].counter < temp){
++tagstore[index][i].counter;
}
}
tagstore[index][j].counter = 0;
return 0;
}

else if((read_miss != currentread_miss) || (write_miss != currentwrite_miss)){
int temp = -1;   
for(i=0; i<L_ASSOC; ++i){
if(tagstore[index][i].counter > temp){
temp = tagstore[index][i].counter;
}
}
for(i=0; i<L_ASSOC; ++i){
if(temp != tagstore[index][i].counter){
tagstore[index][i].counter++;
}
else{
tagstore[index][i].counter = 0;
}
}
currentread_miss = read_miss;
currentwrite_miss = write_miss;
for(i=0; i<L_ASSOC; ++i){
if(tagstore[index][i].counter== 0){
return i;
}   
}
}
//return 0;
}
/*-----------------------------------------------------Display Function-------------------------------------------------*/
void Cache::displaybasic(long index, int L_ASSOC, string L){
cout << endl;

if(L_ASSOC !=1){
cout << "===== " << L << " contents =====" << endl;
for(i=0; i<(pow(2, index)); i++){
cout << "set  " << '\t' << dec << i  << ':' <<'\t';
int k = 0;
int m = 0;
for(int j=0; j<L_ASSOC; j++){
for(int l=0; l<L_ASSOC; l++){
if(tagstore[i][l].counter == m){
tag_store[i][k] = tagstore[i][l].tag;
dirty_store[i][k] = tagstore[i][l].dirtybit;
cout << hex << tag_store[i][k] << " "<< dirty_store[i][k] << " " << "||" << '\t';
}
}    
k++;
m++;
}
cout << endl;
}
}
/***********************************************************/
if(L_ASSOC == 1){
cout << "===== " << L << " contents =====" << endl;
for(i=0; i<(pow(2, index)); i++){
cout << "set  " << '\t' << dec << i  << ':' <<'\t';
for(int j=0; j<L_ASSOC; j++){
cout << hex << tagstore[i][j].tag << " "<< tagstore[i][j].dirtybit << " " << "||" << '\t';
cout <<endl;
}    
}
//cout << endl;
}

/************************************************/
}
/*-----------------------------------------------------Display Function-------------------------------------------------*/
void Cache::displaysector(long index, int L_ASSOC, string L){
cout << endl;
cout << "===== " << L << " Address Array contents =====" << endl;
for(i=0; i<(pow(2, index)); i++){
cout << "set  " << '\t' << dec << i  << ':' <<'\t';
for(int j=0; j<L_ASSOC; j++){
cout << hex << tagstore[i][j].tag << '\t';
}
cout << "||";
cout << endl;
}    
cout << endl;
cout << dec;
cout << "===== " << L << " Data Array contents =====" << endl;
for(i=0; i<(pow(2, index)); i++){
cout << "set  " << '\t' << dec << i  << ':' <<'\t';
for(int j=0; j<L2_DATA_BLOCKS; j++){
cout << dataarray[i][j].selectionbit << ','<< dataarray[i][j].validbit << ',' << dataarray[i][j].dirtybit << '\t'<< '\t';
}
cout << "||";
cout << endl;
}    
}
/*-----------------------------------------Function main starts here--------------------------------------------*/
int main(int argc, char* argv[]) {

int BLOCKSIZE          = stoi(argv[1]);
int L1_SIZE            = stoi(argv[2]);
L1_ASSOC               = stoi(argv[3]);
int L2_SIZE            = stoi(argv[4]);
L2_ASSOC               = stoi(argv[5]);
L2_DATA_BLOCKS         = stoi(argv[6]);
L2_ADDRESS_TAGS        = stoi(argv[7]);
char* trace_file       = argv[8];
ss << trace_file;
ss >> file;
cache_L1.nextLevel = &cache_L2;
cache_L2.nextLevel = nullptr;
/*------------------------------Verify if blocksize is power of 2--------------------------------------*/
flag = isPoweroftwo(BLOCKSIZE);
if (!flag){
cout << "blocksize is not valid" << endl;
}
else{
flag = 0;
}
//cout <<endl;
/*----------------------------------- Display simulator configuration --------------------------------*/
cout << "===== Simulator configuration =====" << endl;
cout << "BLOCKSIZE:" << '\t' << BLOCKSIZE << '\n' << "L1_SIZE:" << '\t' << L1_SIZE << '\n' << "L1_ASSOC:" << '\t' << L1_ASSOC << endl;
cout << "L2_SIZE:" << '\t' << L2_SIZE << '\n' << "L2_ASSOC:" << '\t' << L2_ASSOC << '\n' << "L2_DATA_BLOCKS:" << '\t' << L2_DATA_BLOCKS << '\n' << "L2_ADDRESS_TAGS:" << '\t' << L2_ADDRESS_TAGS << endl;
cout << "trace_file:" << '\t' << trace_file << endl;
/*----------------------------------- Calculate the no: of bits ---------------------------------------*/
blockoffset_bits    = log2(BLOCKSIZE);
index_bits_L1       = log2(L1_SIZE/(BLOCKSIZE*L1_ASSOC));
tag_bits_L1         = 32-(blockoffset_bits+index_bits_L1);
if(!((L2_ADDRESS_TAGS == 0) && (L2_DATA_BLOCKS == 0))){ 
sectoroffset_bits_L2 = log2(L2_DATA_BLOCKS);
index_bits_L2        = log2(L2_SIZE/(BLOCKSIZE*L2_DATA_BLOCKS*L2_ASSOC));
addresstags_bits_L2  = log2(L2_ADDRESS_TAGS);
tag_bits_L2          = 32-(blockoffset_bits+index_bits_L2+sectoroffset_bits_L2+addresstags_bits_L2);
}
/*----------------------------------- Initialize counter, dirtyflag and validflag ---------------------------------*/
for(int i=0; i<(pow(2, index_bits_L1)); ++i){
for(int j=0; j<L1_ASSOC; ++j){
cache_L1.tagstore[i][j].counter = j;
cache_L1.tagstore[i][j].dirtybit = 'N';
cache_L1.tagstore[i][j].validbit = 'I';
}
}
for(int i=0; i<(pow(2, index_bits_L2)); ++i){
for(int j=0; j<(L2_DATA_BLOCKS*L2_ASSOC); ++j){
cache_L2.tagstore[i][j].counter = j;    
cache_L2.dataarray[i][j].dirtybit = 'N';
cache_L2.dataarray[i][j].validbit = 'I';
cache_L2.dataarray[i][j].selectionbit = 0;
}
}

/*----------------------------Read file to fetch and decode address------------------------------------------------*/       
getaddress(file);
/*-----------------------------Function call to display the final display of Cache simulator------------------------*/
if((L2_DATA_BLOCKS == 1) && (L2_ADDRESS_TAGS == 1)){
cache_L1.displaybasic(index_bits_L1, L1_ASSOC, "L1");
cache_L2.displaybasic(index_bits_L2, L2_ASSOC, "L2");
}
else{
cache_L1.displaybasic(index_bits_L1, L1_ASSOC, "L1");
if(!((L2_ADDRESS_TAGS == 0) && (L2_DATA_BLOCKS == 0))){ 
cache_L2.displaysector(index_bits_L2, L2_ASSOC*L2_ADDRESS_TAGS, "L2");
}
}
cout << dec;
cout << endl;                                          /*yestchange*/
cout << "===== Simulation Results =====" << endl;
cout << "a. "<<"number of L1 reads :  " <<'\t' << cache_L1.readaccess << endl;
cout << "b. "<<"number of L1 read misses :  " <<'\t' << cache_L1.read_miss << endl; 
cout << "c. "<<"number of L1 writes :" <<'\t' << cache_L1.writeaccess << endl;
cout << "d. "<<"number of L1 write misses:  " << '\t' << cache_L1.write_miss << endl;
cout << "e. "<<"L1 miss rate:  "<< '\t';
printf ("%0.4f\n",(float)(cache_L1.read_miss + cache_L1.write_miss)/(cache_L1.readaccess + cache_L1.writeaccess));
cout << "f. "<<"number of writebacks from L1 memory:  " << '\t' << cache_L1.writeback <<endl;
if((L2_SIZE == 0) && (L2_ASSOC == 0) && (L2_DATA_BLOCKS == 0) && (L2_ADDRESS_TAGS == 0)){
cout << "g. "<<"total memory traffic:  " << '\t' << cache_L1.read_miss + cache_L1.write_miss + cache_L1.writeback << endl;
}
if(!((L2_ADDRESS_TAGS == 0) && (L2_DATA_BLOCKS == 0))){ 
cout << "g. "<<"number of L2 reads :  " <<'\t' << cache_L2.readaccess << endl;
cout << "h. "<<"number of L2 read misses :  " <<'\t' << cache_L2.read_miss << endl; 
cout << "i. "<<"number of L2 writes :" <<'\t' << cache_L2.writeaccess << endl;
cout << "j. "<<"number of L2 write misses:  " << '\t' << cache_L2.write_miss << endl;
if((L2_ADDRESS_TAGS == 1) && (L2_DATA_BLOCKS == 1)){
cout << "k. "<<"L2 miss rate:  "<< '\t';
printf ("%0.4f\n",(float)(cache_L2.read_miss)/cache_L2.readaccess);
cout << "l. "<<"number of writebacks from L2 memory:  " << '\t' << cache_L2.writeback <<endl;
cout << "m. "<<"total memory traffic:  " << '\t' << cache_L2.read_miss + cache_L2.write_miss + cache_L2.writeback << endl;  
}
else{
cout << "k. "<<"number of L2 sector misses:  " << '\t' << cache_L2.sectormiss<< endl;
cout << "l. "<<"number of L2 cache block misses:  " << '\t' <<(cache_L2.read_miss + cache_L2.write_miss)-cache_L2.sectormiss<< endl;
cout << "m. "<<"L2 miss rate:  "<< '\t';
printf ("%0.4f\n",(float)(cache_L2.read_miss)/cache_L2.readaccess);
cout << "n. "<<"number of writebacks from L2 memory:  " << '\t' << cache_L2.writeback <<endl;
cout << "o. "<<"total memory traffic:  " << '\t' << cache_L2.read_miss + cache_L2.write_miss + cache_L2.writeback << endl; 

}
}
return 0;
}