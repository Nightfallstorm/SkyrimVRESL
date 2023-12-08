#include "DataHandler.h"

using namespace RE;

DataHandler* DataHandler::GetSingleton() {
	return reinterpret_cast<DataHandler*>(RE::TESDataHandler::GetSingleton());
}

