#pragma once

namespace Papyrus
{
	using VM = RE::BSScript::Internal::VirtualMachine;
	using StackID = RE::VMStackID;

	bool Bind(VM* a_vm);
}
