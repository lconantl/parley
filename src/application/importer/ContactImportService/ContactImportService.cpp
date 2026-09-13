#include "ContactImportService.hpp"
#include "application/importer/ContactNormalizer/ContactNormalizer.hpp"

#include <algorithm>
#include <iostream>
#include <stdexcept>

namespace
{
void AssertIsIdentifiable(const Contact& contact)
{
	if (contact.canonicalName.empty() && contact.channels.empty())
	{
		throw std::invalid_argument("У контакта нет ни имени, ни контактных данных");
	}
}

bool HasChannel(const std::vector<ContactChannel>& channels, const ContactChannel& channel)
{
	return std::ranges::any_of(channels, [&channel](const ContactChannel& item) {
		return item.type == channel.type && item.value == channel.value;
	});
}

bool HasSource(const std::vector<ContactSource>& sources, const ContactSource source)
{
	return std::ranges::find(sources, source) != sources.end();
}
} // namespace

ContactImportService::ContactImportService(std::shared_ptr<IContactRepository> repository)
	: m_repository(std::move(repository))
{
}

std::size_t ContactImportService::ImportFrom(const IContactSource& source, const std::filesystem::path& path) const
{
	const std::vector<RawContact> rawContacts = source.Load(path);

	std::size_t imported = 0;
	for (const auto& raw : rawContacts)
	{
		try
		{
			ImportOne(raw);
			++imported;
		}
		catch (const std::exception& exception)
		{
			std::cerr << "[ContactImportService] Пропущена запись: " << exception.what() << std::endl;
		}
	}

	return imported;
}

void ContactImportService::ImportOne(const RawContact& raw) const
{
	const Contact incoming = ContactNormalizer::Normalize(raw);
	AssertIsIdentifiable(incoming);

	std::optional<Contact> existing;
	for (const auto& channel : incoming.channels)
	{
		existing = m_repository->FindByChannel(channel);
		if (existing.has_value())
		{
			break;
		}
	}

	if (existing.has_value())
	{
		m_repository->Upsert(MergeInto(*existing, incoming));
	}
	else
	{
		m_repository->Upsert(incoming);
	}
}

Contact ContactImportService::MergeInto(Contact existing, const Contact& incoming) const
{
	if (existing.canonicalName.empty())
	{
		existing.canonicalName = incoming.canonicalName;
	}
	if (existing.role.empty())
	{
		existing.role = incoming.role;
	}
	if (existing.location.empty())
	{
		existing.location = incoming.location;
	}
	if (existing.notes.empty())
	{
		existing.notes = incoming.notes;
	}

	for (const auto& channel : incoming.channels)
	{
		if (!HasChannel(existing.channels, channel))
		{
			existing.channels.push_back(channel);
		}
	}

	for (const auto& source : incoming.sources)
	{
		if (!HasSource(existing.sources, source))
		{
			existing.sources.push_back(source);
		}
	}

	return existing;
}
