#include <iostream>
#include <mpi.h>
#include <string>
#include <fstream>
#include <vector>
#include <chrono>

using namespace std;

/**
 * Ak je nastavený TIME na true tak tak sa namiesto výpisu matice vypíše čas v milicekundách 
*/
#define TIME false

#define MPI_VALUE_TAG 0
#define FIRST_PROCESS 0

/**
 * zistí veľkos matice 
 * @param filename nazov suboru
 * @param rows velkosť matice
 * @param cols velkosť matice
 */ 
void getMatrixSize(string filename,int &rows, int &cols)
{
    ifstream file(filename.c_str());
    if (file.is_open() == false) // kontrola či sa dá otvoriť
    {
        std::cerr << "Input file is bad" << std::endl;
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    string line;
    getline(file,line);
    int count = 0;
    if (filename == "mat1") // mat1 obashuje na prvom riadku počer riadokov
    {
        rows = stoi(line);
        while (file >> line)
            count++;
        cols = count/rows;
    }
    else // mat2 obashuje na prvom riadku počer stĺpcov
    {
        cols = stoi(line);
        while (getline(file,line))
            count++;
        rows = count;
    }
}

/**
 * rozdelí string na základe medzier
 * @param s vstupný string
 * @param v výstupný vektor
 */ 
void SplitString(string s, vector<string> &v){
	string temp = "";
	for(int i=0;i<s.length();++i){
		
		if(s[i]==' '){
			v.push_back(temp);
			temp = "";
		}
		else{
			temp.push_back(s[i]);
		}
	}
	v.push_back(temp);
}

/**
 * vráti hodnotu z matice na pozíciach (i,j)
 * @param filename názov vstupného suboru
 * @param i pozícia i hodnoty zo súbora
 * @param j pozícia j odnoty zo súbora
 * @return hodnota matice
 */
int getValueFromFile(string filename, int i, int j)
{
    ifstream file(filename.c_str());  
    string line;
    vector<string> values;
    int count = -1;
    while (getline(file,line))
    {
        if (count == j)
        {
            SplitString(line,values);
            break;
        }
        count++;
    }
    file.close();
    return stoi(values[i]);
}

/**
 * vráti i-tu pozíciu výstupnej matiec podľa id procesora
 * @param id id procesora
 * @param cols stĺpec matice
 * @return i-tu pozíciu výstupnej matiec
 */ 
int getMatrixIIndex(int id, int cols)
{
    return id/cols;
}

/**
 * vráti j-tu pozíciu výstupnej matiec podľa id procesora
 * @param id id procesora
 * @param cols stĺpec matice 
 * @return j-tu pozíciu výstupnej matiec
 */ 
int getMatrixJIndex(int id, int cols)
{
    return id%cols;
}

/**
 * mapuje 2D maticu do 1D
 * @param i horizontalna pozícia
 * @param j vertikalna pozícia
 * @param cols počet riadkov matice
 * @return id procesu
*/
int getMatrixID(int i, int j, int cols)
{
    return (i * cols) + j;
}

/**
 * vypíše maticu o rozmerovch rows/cols z hodnotami vo vektore v
 * @param v vektor hodnôt na výpis 
 * @param rows Riadky výstupenj matice
 * @param cols Stĺpce výstupenj matice
 */
void PrintMatrix(const std::vector<int>& v, int rows, int cols)
{
    cout << rows << ":" << cols << endl;
    for (int i = 0; i < v.size()-1; i++)
    {
        cout << v[i];
        if ((i+1)%(cols) == 0) //vždy keď sa napíšu všetky čísla zo stĺpca, vypíše sa nový riadok
            cout << endl;
        else
            cout << " ";
        
    }
    cout << v[v.size()-1]; // výpis posledného prvu na konci
    std::cout << std::endl << std::flush;
}

int main(int argc, char *argv[])
{
    int processesCount;
    int processID;

    MPI_Init(&argc, &argv); 
    MPI_Comm_size(MPI_COMM_WORLD,&processesCount);
    MPI_Comm_rank(MPI_COMM_WORLD,&processID); 
    MPI_Status stat;

    int rowsA; int rowsB;
    int colsA; int colsB;
    int registerC = 0;
    getMatrixSize("mat1",rowsA,colsA);
    getMatrixSize("mat2",rowsB,colsB);
     
    /**********************************************************************/
    /**************************** PRVY PROCES *****************************/
    /**********************************************************************/
    if (processID == 0)
    {
        if (colsA != rowsB)
        {
            std::cerr << "Počet stĺpcov prvej matice sa musí rovnať počtu riadkov druhej matice" << std::endl;
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        auto start = std::chrono::high_resolution_clock::now();

        for (int i = 0; i < colsA; i++)
        {
            int valA = getValueFromFile("mat1",i,0);
            int valB = getValueFromFile("mat2",0,i);
            MPI_Send(&valA, 1, MPI_INT, 1, MPI_VALUE_TAG, MPI_COMM_WORLD); //pošle hodnotu procesu na pravo
            MPI_Send(&valB, 1, MPI_INT,getMatrixID(1,0,colsB) , MPI_VALUE_TAG, MPI_COMM_WORLD); //pošle hodnotu procesu na dol
            registerC += valA*valB;
        }

        //prvý proces zozbeira všetky hodnoty od ostatných procesorov a vypíše ich
        vector<int> allData;
        allData.push_back(registerC);
        int recvValue;
        for (int i = 1; i < processesCount; i++)
        {
            MPI_Recv(&recvValue, 1, MPI_INT, i, MPI_VALUE_TAG, MPI_COMM_WORLD, &stat);
            allData.push_back(recvValue);
        }
        auto finish = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = finish - start;
        if (TIME == true)
            std::cout << elapsed.count() * 1000000 << "\n";
        else
            PrintMatrix(allData,rowsA,colsB);
        MPI_Finalize();
        return 0;
    }

    /**********************************************************************/
    /**********************************************************************/
    /**********************************************************************/

    int iIndex = getMatrixIIndex(processID,colsB);
    int jIndex = getMatrixJIndex(processID,colsB);
    
    /**********************************************************************/
    /************************** PROCESY S I == 0 **************************/
    /**********************************************************************/

    if (iIndex == 0)
    {
        int valA;
        int valB;
        for (int i = 0; i < colsA; i++)
        {
            valB = getValueFromFile("mat2",jIndex,i);

            MPI_Recv(&valA, 1, MPI_INT, getMatrixID(iIndex,jIndex-1,colsB), MPI_VALUE_TAG, MPI_COMM_WORLD, &stat);
            
            if (jIndex+1 != colsB)
                MPI_Send(&valA, 1, MPI_INT, getMatrixID(iIndex,jIndex+1,colsB), MPI_VALUE_TAG, MPI_COMM_WORLD); //pošle hodnotu procesu na pravo
            MPI_Send(&valB, 1, MPI_INT, getMatrixID(iIndex+1,jIndex,colsB) , MPI_VALUE_TAG, MPI_COMM_WORLD); //pošle hodnotu procesu na dol
            registerC += valA*valB;
        }
        //cout << "hodnota " << processID << " je: " << registerC << endl;
        MPI_Send(&registerC, 1, MPI_INT, FIRST_PROCESS, MPI_VALUE_TAG, MPI_COMM_WORLD);
        MPI_Finalize();
        return 0;
    }
    
    /**********************************************************************/
    /**********************************************************************/
    /**********************************************************************/

    /**********************************************************************/
    /************************** PROCESY S J == 0 **************************/
    /**********************************************************************/
    if (jIndex == 0)
    {
        int valA;
        int valB;
        for (int i = 0; i < colsA; i++)
        {
            valA = getValueFromFile("mat1",i,iIndex);
            
            MPI_Recv(&valB, 1, MPI_INT, getMatrixID(iIndex-1,jIndex,colsB), MPI_VALUE_TAG, MPI_COMM_WORLD, &stat);

            MPI_Send(&valA, 1, MPI_INT, getMatrixID(iIndex,jIndex+1,colsB), MPI_VALUE_TAG, MPI_COMM_WORLD); //pošle hodnotu procesu na pravo
            if (iIndex+1 != rowsA)
                MPI_Send(&valB, 1, MPI_INT, getMatrixID(iIndex+1,jIndex,colsB) , MPI_VALUE_TAG, MPI_COMM_WORLD); //pošle hodnotu procesu na dol
            registerC += valA*valB;
        }
        //cout << "hodnota " << processID << " je: " << registerC << endl;
        MPI_Send(&registerC, 1, MPI_INT, FIRST_PROCESS, MPI_VALUE_TAG, MPI_COMM_WORLD);
        MPI_Finalize();
        return 0;
    }
    /**********************************************************************/
    /**********************************************************************/
    /**********************************************************************/

    /**********************************************************************/
    /********************** VŠETKY OSTATNÉ PROCESY  ***********************/
    /**********************************************************************/

    for (int i = 0; i < colsA; i++)
        {
            int valA;
            int valB;

            MPI_Recv(&valA, 1, MPI_INT, getMatrixID(iIndex,jIndex-1,colsB), MPI_VALUE_TAG, MPI_COMM_WORLD, &stat);
            MPI_Recv(&valB, 1, MPI_INT, getMatrixID(iIndex-1,jIndex,colsB), MPI_VALUE_TAG, MPI_COMM_WORLD, &stat);

            if (jIndex+1 != colsB)
                MPI_Send(&valA, 1, MPI_INT, getMatrixID(iIndex,jIndex+1,colsB), MPI_VALUE_TAG, MPI_COMM_WORLD); //pošle hodnotu procesu na pravo   
            if (iIndex+1 != rowsA)
                MPI_Send(&valB, 1, MPI_INT, getMatrixID(iIndex+1,jIndex,colsB) , MPI_VALUE_TAG, MPI_COMM_WORLD); //pošle hodnotu procesu na dol
            registerC += valA*valB;
        }

    MPI_Send(&registerC, 1, MPI_INT, FIRST_PROCESS, MPI_VALUE_TAG, MPI_COMM_WORLD);
    /**********************************************************************/
    /**********************************************************************/
    /**********************************************************************/

    MPI_Finalize();
    return 0;
}