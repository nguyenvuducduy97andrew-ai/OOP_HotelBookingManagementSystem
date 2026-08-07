#include "CustomerManager.h"

#include <QDate>
#include <QDateTime>
#include <QList>
#include <QRegularExpression>
#include <QString>
#include <QStringList>

#include <algorithm>
#include <utility>

#include "CustomerIdentity.h"
#include "customer/CountryInputRules.h"

namespace {
std::string collapseWhitespace(const std::string& value)
{
	return QString::fromStdString(value).simplified().toStdString();
}

bool isSingleNameTokenValid(const QString& token)
{
	static const QRegularExpression tokenPattern(QStringLiteral(R"(^[\p{L}][\p{L}'’\-.]*$)"));
	return tokenPattern.match(token).hasMatch();
}

bool isValidCustomerIdFormat(const std::string& customerId)
{
	const QString id = QString::fromStdString(customerId).trimmed().toUpper();
	const QStringList identityParts = id.split('|');
	if (identityParts.size() == 3) {
		return isValidDocumentNumber(identityParts.at(0), identityParts.at(1), identityParts.at(2));
	}

	static const QList<QRegularExpression> idPatterns = {
		QRegularExpression(QStringLiteral(R"(^\d{12}$)")),
		QRegularExpression(QStringLiteral(R"(^\d{9}$)")),
		QRegularExpression(QStringLiteral(R"(^[A-CEGHJ-PR-TW-Z]{2}\d{6}[A-D]$)")),
		QRegularExpression(QStringLiteral(R"(^[STFGM]\d{7}[A-Z]$)")),
		QRegularExpression(QStringLiteral(R"(^\d{13}$)")),
		QRegularExpression(QStringLiteral(R"(^\d{10}$)")),
		QRegularExpression(QStringLiteral(R"(^[A-Z0-9]{9}$)"))
	};

	return std::any_of(idPatterns.cbegin(), idPatterns.cend(), [&id](const QRegularExpression& pattern) {
		return pattern.match(id).hasMatch();
	});
}

bool isValidCustomerNameFormat(const std::string& customerName)
{
	const QString normalized = QString::fromStdString(collapseWhitespace(customerName));
	if (normalized.isEmpty() || normalized.size() > 120) {
		return false;
	}

	const QStringList tokens = normalized.split(' ', Qt::SkipEmptyParts);
	if (tokens.isEmpty()) {
		return false;
	}

	for (const QString& token : tokens) {
		if (!isSingleNameTokenValid(token)) {
			return false;
		}
	}
	return true;
}

bool isValidPhoneNumberFormat(const std::string& phoneNumber)
{
	const QString phone = QString::fromStdString(collapseWhitespace(phoneNumber));
	static const QRegularExpression phonePattern(QStringLiteral(R"(^\+[1-9]\d{7,14}$)"));
	return phonePattern.match(phone).hasMatch();
}
}

CustomerManager::CustomerManager() = default;

const std::vector<std::shared_ptr<Customer>>& CustomerManager::getCustomers() const
{
	return m_customers;
}

std::shared_ptr<Customer> CustomerManager::findCustomerById(const std::string& customerId) const
{
	for (const auto& customer : m_customers) {
		if (customer && customer->getCustomerId() == customerId) {
			return customer;
		}
	}
	return nullptr;
}

bool CustomerManager::customerIdExists(const std::string& customerId) const
{
	return findCustomerById(customerId) != nullptr;
}

bool CustomerManager::registerCustomer(const std::string& id, const std::string& name, const std::string& phone,
									  std::string& errorMessage, std::string* conflictingCustomerId)
{
	if (conflictingCustomerId) {
		conflictingCustomerId->clear();
	}

	const QString inputName = QString::fromStdString(collapseWhitespace(name));
	std::shared_ptr<Customer> phoneMatch;
	std::shared_ptr<Customer> idMatch;

	for (const auto& existing : m_customers) {
		if (!existing) {
			continue;
		}
		if (!phoneMatch && existing->getPhoneNumber() == phone) {
			phoneMatch = existing;
		}
		if (!idMatch && existing->getCustomerId() == id) {
			idMatch = existing;
		}
	}

	const auto reportConflict = [&conflictingCustomerId](const std::shared_ptr<Customer>& customer) {
		if (conflictingCustomerId && customer) {
			*conflictingCustomerId = customer->getCustomerId();
		}
	};

	if (phoneMatch) {
		const bool sameName = QString::fromStdString(collapseWhitespace(phoneMatch->getName())).compare(inputName, Qt::CaseInsensitive) == 0;
		if (sameName && phoneMatch->getCustomerId() == id) {
			reportConflict(phoneMatch);
			errorMessage = "This customer already exists.";
			return false;
		}

		reportConflict(phoneMatch);
		errorMessage = "This phone number is already used by another customer. Please use a different number.";
		return false;
	}

	if (idMatch) {
		reportConflict(idMatch);
		errorMessage = "This ID number is already linked to another customer account. Please verify and enter it again.";
		return false;
	}

	if (!isValidCustomerIdFormat(id)) {
		errorMessage = "Customer ID does not match a supported national ID format.";
		return false;
	}

	if (!isValidCustomerNameFormat(name)) {
		errorMessage = "Customer name must be a valid legal name using letters, spaces, apostrophes, hyphens, or initials.";
		return false;
	}

	if (!isValidPhoneNumberFormat(phone)) {
		errorMessage = "Phone number does not match the selected country format.";
		return false;
	}

	if (customerIdExists(id)) {
		errorMessage = "Customer ID already exists.";
		return false;
	}

	auto customer = std::make_shared<Customer>();
	customer->setCustomerId(id);
	const QStringList identityParts = QString::fromStdString(id).split('|');
	if (identityParts.size() == 3) {
		customer->setDocumentType(identityParts.at(0).toStdString());
		customer->setIssuingCountry(identityParts.at(1).toStdString());
		customer->setDocumentNumber(identityParts.at(2).toStdString());
	} else {
		customer->setDocumentType("National ID");
		customer->setIssuingCountry("Legacy");
		customer->setDocumentNumber(id);
	}
	customer->setName(name);
	customer->setPhoneNumber(phone);

	m_customers.push_back(customer);
	return true;
}

bool CustomerManager::updateCustomer(const std::string& customerId, const std::string& name, const std::string& phone,
									 std::string& errorMessage, std::string* conflictingCustomerId)
{
	if (conflictingCustomerId) {
		conflictingCustomerId->clear();
	}

	if (!findCustomerById(customerId)) {
		errorMessage = "Customer not found.";
		return false;
	}

	const QString inputName = QString::fromStdString(collapseWhitespace(name));
	for (const auto& existing : m_customers) {
		if (!existing || existing->getCustomerId() == customerId || existing->getPhoneNumber() != phone) {
			continue;
		}

		if (conflictingCustomerId) {
			*conflictingCustomerId = existing->getCustomerId();
		}
		if (QString::fromStdString(collapseWhitespace(existing->getName())).compare(inputName, Qt::CaseInsensitive) == 0) {
			errorMessage = "This customer already exists.";
		} else {
			errorMessage = "This phone number is already used by another customer. Please use a different number.";
		}
		return false;
	}

	if (!isValidCustomerNameFormat(name)) {
		errorMessage = "Customer name must be a valid legal name using letters, spaces, apostrophes, hyphens, or initials.";
		return false;
	}

	if (!isValidPhoneNumberFormat(phone)) {
		errorMessage = "Phone number does not match the selected country format.";
		return false;
	}

	const auto customer = findCustomerById(customerId);
	customer->setName(name);
	customer->setPhoneNumber(phone);
	return true;
}

bool CustomerManager::resolveForBooking(const std::string& id, const std::string& name, const std::string& phone, std::string& errorMessage)
{
	auto customer = findCustomerById(id);
	if (!customer) {
		return registerCustomer(id, name, phone, errorMessage);
	}

	if (customer->isArchived()) {
		errorMessage = "Archived customers cannot be used for bookings.";
		return false;
	}

	const QString storedName = QString::fromStdString(customer->getName()).simplified();
	const QString inputName = QString::fromStdString(collapseWhitespace(name)).simplified();
	if (storedName.compare(inputName, Qt::CaseInsensitive) != 0) {
		errorMessage = "Customer name does not match the registered customer for this ID.";
		return false;
	}

	if (customer->getPhoneNumber() != phone) {
		errorMessage = "Phone number does not match the registered customer for this ID.";
		return false;
	}

	return true;
}

bool CustomerManager::archiveCustomer(const std::string& customerId, std::string& errorMessage)
{
	auto customer = findCustomerById(customerId);
	if (!customer) {
		errorMessage = "Customer not found.";
		return false;
	}

	customer->setArchived(true);
	return true;
}

bool CustomerManager::restoreCustomer(const std::string& customerId, std::string& errorMessage)
{
	auto customer = findCustomerById(customerId);
	if (!customer) {
		errorMessage = "Customer not found.";
		return false;
	}

	customer->setArchived(false);
	return true;
}

bool CustomerManager::deleteCustomer(const std::string& customerId, std::string& errorMessage)
{
	auto customer = findCustomerById(customerId);
	if (!customer) {
		errorMessage = "Customer not found.";
		return false;
	}

	m_customers.erase(std::remove(m_customers.begin(), m_customers.end(), customer), m_customers.end());
	return true;
}

bool CustomerManager::restoreCustomerFromDatabase(const std::string& customerId,
												  const std::string& documentType,
												  const std::string& issuingCountry,
												  const std::string& documentNumber,
												  const std::string& name,
												  const std::string& phone,
												  bool archived,
												  std::string& errorMessage)
{
	if (customerId.empty()) {
		errorMessage = "Persisted customer ID is empty.";
		return false;
	}

	if (customerIdExists(customerId)) {
		errorMessage = "Duplicate persisted customer ID: " + customerId;
		return false;
	}

	if ((!isValidCustomerIdFormat(customerId) && issuingCountry != "Legacy") || !isValidCustomerNameFormat(name) || !isValidPhoneNumberFormat(phone)) {
		errorMessage = "Persisted customer record has an invalid ID, name, or phone number.";
		return false;
	}

	for (const auto& existing : m_customers) {
		if (existing && existing->getPhoneNumber() == phone) {
			errorMessage = "Duplicate persisted customer phone number: " + phone;
			return false;
		}
	}

	auto customer = std::make_shared<Customer>();
	customer->setCustomerId(customerId);
	customer->setDocumentType(documentType.empty() ? "National ID" : documentType);
	customer->setIssuingCountry(issuingCountry.empty() ? "Legacy" : issuingCountry);
	customer->setDocumentNumber(documentNumber.empty() ? customerId : documentNumber);
	customer->setName(name);
	customer->setPhoneNumber(phone);
	customer->setArchived(archived);

	m_customers.push_back(customer);
	return true;
}

void CustomerManager::clearAll()
{
	m_customers.clear();
}
