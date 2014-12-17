#include <fstream>
#include <vector>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string.h>

using namespace std;

bool getFileContent(char* fileName, vector<unsigned char> &fileContent);
vector<unsigned char> xor_bytes_bytes(vector<unsigned char> a, vector<unsigned char> b);

vector<unsigned char> f(vector<unsigned char> Hin, vector<unsigned char> m);
vector< vector<unsigned char> > genKeys(vector<unsigned char> U, vector<unsigned char> V);
vector<unsigned char> A(vector<unsigned char> Y);
vector<unsigned char> P(vector<unsigned char> Y);
vector<unsigned char> E(vector<unsigned char> D, vector<unsigned char> K);
vector<unsigned char> E_f(vector<unsigned char> A, vector<unsigned char> K);
vector<unsigned char> psi(vector<unsigned char> Y, int n);

vector<unsigned char> checkSum(vector<unsigned char> sum, vector<unsigned char> m);
const char* getHash(vector<unsigned char> M);


int main(int argc, char* argv[])
{
	vector<unsigned char> in;
	getFileContent(argv[1], in);
	cout << getHash(in) << endl;
	return 0;
}

bool getFileContent(char* fileName, vector<unsigned char> &fileContent)
{
	ifstream inputStream(fileName, std::ios::binary);
	if (inputStream.fail())
		return false;

	inputStream.seekg(0, std::ios::end);
	int inputSize = inputStream.tellg();
	inputStream.seekg(0, std::ios::beg);

	if (!inputSize)
	{
		fileContent.clear();
		return true;
	}

	fileContent.resize(inputSize);
	inputStream.read((char*)&fileContent.front(), inputSize);
	inputStream.close();

	return true;
}

// xor двух байтовых массивов
// никаких проверок
vector<unsigned char> xor_bytes(vector<unsigned char> a, vector<unsigned char> b)
{
	vector<unsigned char> res(a.size());
	for (int i = 0; i < a.size(); i++)
		res[i] = a[i] ^ b[i];
	return res;
}

const char* getHash(vector<unsigned char> M)
{
	// инициализация
	vector<unsigned char> h(32);
	vector<unsigned char> sum(32);
	vector<unsigned char> L(32);

	// Функция сжатия внутренних итераций: для i = 1 … n — 1
	for (int i = 0; i < M.size() / 32; i++)
	{
		// итерация метода последовательного хеширования
		vector<unsigned char> Mi;
		Mi.insert(Mi.begin(), M.begin() + i * 32, M.begin() + (i + 1) * 32);
		h = f(h, Mi);

		// итерация вычисления контрольной суммы
		sum = checkSum(sum, Mi);
	}
	L[M.size() / 32] = 1;
	// функция финальной итерации
	if (M.size() % 32)
	{
		L[0] = (M.size() % 32) * 8;
		vector<unsigned char> Mi;
		Mi.insert(Mi.begin(), M.end() - M.size() % 32, M.end());
		Mi.insert(Mi.end(), 32 - M.size() % 32, 0);
		h = f(h, Mi);
		sum = checkSum(sum, Mi);
	}

	h = f(h, L);
	h = f(h, sum);

	ostringstream res;
	res.fill('0');
	for (int i = 0; i < 32; i++)
	{
		res << std::hex << setw(2) << (int)h[i];
	}

	char* result = new char[65];
	strcpy(result, (char*)res.str().c_str());
	return result;
}

//  Шаговая функция хеширования
vector<unsigned char> f(vector<unsigned char> Hin, vector<unsigned char> m)
{
	// Генерация ключей
	vector< vector<unsigned char> > keys = genKeys(Hin, m);
	
	// шифрующее преобразование
	vector<unsigned char> S;
	for (int i = 0; i < 4; i++)
	{
		vector<unsigned char> tmp;
		tmp.insert(tmp.end(), Hin.begin() + i * 8, Hin.begin() + (i + 1) * 8);
		tmp = E(tmp, keys[i]);
		S.insert(S.end(), tmp.begin(), tmp.end());
	}

	// перемешивающее преобразование
	return psi(xor_bytes(Hin, psi(xor_bytes(m, psi(S, 12)), 1)), 61);
}

vector<vector<unsigned char> > genKeys(vector<unsigned char> U, vector<unsigned char> V)
{
	static const unsigned char Y[] = {
		0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff,
		0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 
		0x00, 0xff, 0xff, 0x00, 0xff, 0x00, 0x00, 0xff, 
		0xff, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0xff};
	vector<unsigned char> C3(Y, Y + sizeof(Y) / sizeof(Y[0]));

	vector<vector<unsigned char> > K(4);
	vector<unsigned char> W = xor_bytes(U, V);
	K[0] = P(W);
	for (int j = 1; j < 4; j++)
	{
		U = A(U);
		if (j == 2)
			U = xor_bytes(U, C3);
		V = A(A(V));
		W = xor_bytes(U, V);
		K[j] = P(W);
	}

	return K;
}

vector<unsigned char> A(vector<unsigned char> Y)
{
	vector<unsigned char> res(32);
	for (int i = 0; i < 24; i++)
		res[i] = Y[i + 8];

	for (int i = 0; i < 8; i++)
		res[i + 24] = Y[i] ^ Y[i + 8];
	
	return res;
}

vector<unsigned char> P(vector<unsigned char> Y)
{
	vector<unsigned char> res(32);
	for (int i = 0; i <= 3; i++)
		for (int k = 1; k <= 8; k++)
			res[i + 1 + 4*(k-1) -1] = Y[8*i + k -1];
	
	return res;
}

// Шифрование из ГОСТ 28147-89
vector<unsigned char> E(vector<unsigned char> D, vector<unsigned char> K)
{
	vector<vector<unsigned char> > keys; // разобъём 32-байтный ключ на 8 4-байтныйх
	for (int i = 0; i < 8; i++)
	{
		vector<unsigned char> tmp;
		tmp.insert(tmp.end(), K.begin() + i * 4, K.begin() + (i+1)*4);
		keys.push_back(tmp);
	}
	vector<unsigned char> A, B;
	A.insert(A.end(), D.begin(), D.begin() + 4);
	B.insert(B.end(), D.begin() + 4, D.end());

	for (int j = 0; j < 3; j++)
	{
		for (int i = 0; i < 8; i++)
		{
			vector<unsigned char> tmp = E_f(A, keys[i]);
			tmp = xor_bytes(tmp, B);
			B = A;
			A = tmp;
		}
	}

	for (int i = 7; i >= 0; i--)
	{
		vector<unsigned char> tmp = E_f(A, keys[i]);
		tmp = xor_bytes(tmp, B);
		B = A;
		A = tmp;
	}

	vector<unsigned char> res;
	res.insert(res.end(), B.begin(), B.end());
	res.insert(res.end(), A.begin(), A.end());
	
	return res;
}

// один шаг из шифрования ГОСТ 28147 - 89
vector<unsigned char> E_f(vector<unsigned char> A, vector<unsigned char> K)
{
	static unsigned char S[8][16] = {
		{ 4, 10, 9, 2, 13, 8, 0, 14, 6, 11, 1, 12, 7, 15, 5, 3 },
		{ 14, 11, 4, 12, 6, 13, 15, 10, 2, 3, 8, 1, 0, 7, 5, 9 },
		{ 5, 8, 1, 13, 10, 3, 4, 2, 14, 15, 12, 7, 6, 0, 9, 11 },
		{ 7, 13, 10, 1, 0, 8, 9, 15, 14, 4, 6, 12, 11, 2, 5, 3 },
		{ 6, 12, 7, 1, 5, 15, 13, 8, 4, 10, 9, 14, 0, 3, 11, 2 },
		{ 4, 11, 10, 0, 7, 2, 1, 13, 3, 6, 8, 5, 9, 12, 15, 14 },
		{ 13, 11, 4, 1, 3, 15, 5, 9, 0, 10, 14, 7, 6, 8, 2, 12 },
		{ 1, 15, 13, 0, 5, 7, 10, 4, 9, 2, 3, 14, 6, 11, 8, 12 },
	};

	vector<unsigned char> res(4);
	int c = 0;
	for (int i = 0; i < 4; i++)
	{
		c += A[i] + K[i];
		res[i] = c & 0xFF;
		c >>= 8;
	}

	for (int i = 0; i < 8; i++)
	{
		int x = res[i >> 1] & ((i & 1) ? 0xF0 : 0x0F);

		res[i >> 1] ^= x;
		x >>= (i & 1) ? 4 : 0;
		x = S[i][x];
		res[i >> 1] |= x << ((i & 1) ? 4 : 0);
	}

	int tmp = res[3];
	res[3] = res[2];
	res[2] = res[1];
	res[1] = res[0];
	res[0] = tmp;
	tmp = res[0] >> 5;

	for (int i = 1; i < 4; i++)
	{
		int nTmp = res[i] >> 5;
		res[i] = (res[i] << 3) | tmp;
		tmp = nTmp;
	}

	res[0] = (res[0] << 3) | tmp;
	return res;
}

vector<unsigned char> psi(vector<unsigned char> Y, int n)
{
	for (int i = 0; i < n; i++)
	{
		unsigned char tmp[] = { 0, 0 };

		static const char indexes[] = { 1, 2, 3, 4, 13, 16 };
		for (int j = 0; j < sizeof(indexes); j++)
		{
			tmp[0] ^= Y[2 * (indexes[j] - 1)];
			tmp[1] ^= Y[2 * (indexes[j] - 1) + 1];
		}

		for (int i = 0; i < 30; i++)
			Y[i] = Y[i + 2];

		Y[30] = tmp[0];
		Y[31] = tmp[1];
	}
	return Y;
}

vector<unsigned char> checkSum(vector<unsigned char> sum, vector<unsigned char> m)
{
	int carry = 0;
	for (int i = 0; i < 32; i++)
	{
		int t = (int)sum[i] + (int)m[i] + carry;
		if (t > 256)
			carry = 1;
		else
			carry = 0;
		sum[i] = t;
	}

	return sum;
}
