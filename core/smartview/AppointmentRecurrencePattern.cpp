#include <core/stdafx.h>
#include <core/smartview/AppointmentRecurrencePattern.h>
#include <core/smartview/SmartView.h>
#include <core/interpret/flags.h>
#include <core/mapi/extraPropTags.h>
#include <core/interpret/guid.h>

namespace smartview
{
	void AppointmentRecurrencePattern::Parse()
	{
		m_RecurrencePattern.parse(m_Parser, false);

		m_ReaderVersion2 = m_Parser.Get<DWORD>();
		m_WriterVersion2 = m_Parser.Get<DWORD>();
		m_StartTimeOffset = m_Parser.Get<DWORD>();
		m_EndTimeOffset = m_Parser.Get<DWORD>();
		m_ExceptionCount = m_Parser.Get<WORD>();

		if (m_ExceptionCount && m_ExceptionCount == m_RecurrencePattern.m_ModifiedInstanceCount &&
			m_ExceptionCount < _MaxEntriesSmall)
		{
			m_ExceptionInfo.reserve(m_ExceptionCount);
			for (WORD i = 0; i < m_ExceptionCount; i++)
			{
				ExceptionInfo exceptionInfo;
				exceptionInfo.StartDateTime = m_Parser.Get<DWORD>();
				exceptionInfo.EndDateTime = m_Parser.Get<DWORD>();
				exceptionInfo.OriginalStartDate = m_Parser.Get<DWORD>();
				exceptionInfo.OverrideFlags = m_Parser.Get<WORD>();
				if (exceptionInfo.OverrideFlags & ARO_SUBJECT)
				{
					exceptionInfo.SubjectLength = m_Parser.Get<WORD>();
					exceptionInfo.SubjectLength2 = m_Parser.Get<WORD>();
					if (exceptionInfo.SubjectLength2 && exceptionInfo.SubjectLength2 + 1 == exceptionInfo.SubjectLength)
					{
						exceptionInfo.Subject = m_Parser.GetStringA(exceptionInfo.SubjectLength2);
					}
				}

				if (exceptionInfo.OverrideFlags & ARO_MEETINGTYPE)
				{
					exceptionInfo.MeetingType = m_Parser.Get<DWORD>();
				}

				if (exceptionInfo.OverrideFlags & ARO_REMINDERDELTA)
				{
					exceptionInfo.ReminderDelta = m_Parser.Get<DWORD>();
				}
				if (exceptionInfo.OverrideFlags & ARO_REMINDER)
				{
					exceptionInfo.ReminderSet = m_Parser.Get<DWORD>();
				}

				if (exceptionInfo.OverrideFlags & ARO_LOCATION)
				{
					exceptionInfo.LocationLength = m_Parser.Get<WORD>();
					exceptionInfo.LocationLength2 = m_Parser.Get<WORD>();
					if (exceptionInfo.LocationLength2 &&
						exceptionInfo.LocationLength2 + 1 == exceptionInfo.LocationLength)
					{
						exceptionInfo.Location = m_Parser.GetStringA(exceptionInfo.LocationLength2);
					}
				}

				if (exceptionInfo.OverrideFlags & ARO_BUSYSTATUS)
				{
					exceptionInfo.BusyStatus = m_Parser.Get<DWORD>();
				}

				if (exceptionInfo.OverrideFlags & ARO_ATTACHMENT)
				{
					exceptionInfo.Attachment = m_Parser.Get<DWORD>();
				}

				if (exceptionInfo.OverrideFlags & ARO_SUBTYPE)
				{
					exceptionInfo.SubType = m_Parser.Get<DWORD>();
				}

				if (exceptionInfo.OverrideFlags & ARO_APPTCOLOR)
				{
					exceptionInfo.AppointmentColor = m_Parser.Get<DWORD>();
				}

				m_ExceptionInfo.push_back(exceptionInfo);
			}
		}

		m_ReservedBlock1Size = m_Parser.Get<DWORD>();
		m_ReservedBlock1 = m_Parser.GetBYTES(m_ReservedBlock1Size, _MaxBytes);

		if (m_ExceptionCount && m_ExceptionCount == m_RecurrencePattern.m_ModifiedInstanceCount &&
			m_ExceptionCount < _MaxEntriesSmall && !m_ExceptionInfo.empty())
		{
			for (WORD i = 0; i < m_ExceptionCount; i++)
			{
				ExtendedException extendedException;

				std::vector<BYTE> ReservedBlockEE2;
				if (m_WriterVersion2 >= 0x0003009)
				{
					extendedException.ChangeHighlight.ChangeHighlightSize = m_Parser.Get<DWORD>();
					extendedException.ChangeHighlight.ChangeHighlightValue = m_Parser.Get<DWORD>();
					if (extendedException.ChangeHighlight.ChangeHighlightSize > sizeof(DWORD))
					{
						extendedException.ChangeHighlight.Reserved = m_Parser.GetBYTES(
							extendedException.ChangeHighlight.ChangeHighlightSize - sizeof(DWORD), _MaxBytes);
					}
				}

				extendedException.ReservedBlockEE1Size = m_Parser.Get<DWORD>();
				extendedException.ReservedBlockEE1 =
					m_Parser.GetBYTES(extendedException.ReservedBlockEE1Size, _MaxBytes);

				if (m_ExceptionInfo[i].OverrideFlags & ARO_SUBJECT || m_ExceptionInfo[i].OverrideFlags & ARO_LOCATION)
				{
					extendedException.StartDateTime = m_Parser.Get<DWORD>();
					extendedException.EndDateTime = m_Parser.Get<DWORD>();
					extendedException.OriginalStartDate = m_Parser.Get<DWORD>();
				}

				if (m_ExceptionInfo[i].OverrideFlags & ARO_SUBJECT)
				{
					extendedException.WideCharSubjectLength = m_Parser.Get<WORD>();
					if (extendedException.WideCharSubjectLength)
					{
						extendedException.WideCharSubject =
							m_Parser.GetStringW(extendedException.WideCharSubjectLength);
					}
				}

				if (m_ExceptionInfo[i].OverrideFlags & ARO_LOCATION)
				{
					extendedException.WideCharLocationLength = m_Parser.Get<WORD>();
					if (extendedException.WideCharLocationLength)
					{
						extendedException.WideCharLocation =
							m_Parser.GetStringW(extendedException.WideCharLocationLength);
					}
				}

				if (m_ExceptionInfo[i].OverrideFlags & ARO_SUBJECT || m_ExceptionInfo[i].OverrideFlags & ARO_LOCATION)
				{
					extendedException.ReservedBlockEE2Size = m_Parser.Get<DWORD>();
					extendedException.ReservedBlockEE2 =
						m_Parser.GetBYTES(extendedException.ReservedBlockEE2Size, _MaxBytes);
				}

				m_ExtendedException.push_back(extendedException);
			}
		}

		m_ReservedBlock2Size = m_Parser.Get<DWORD>();
		m_ReservedBlock2 = m_Parser.GetBYTES(m_ReservedBlock2Size, _MaxBytes);
	}

	void AppointmentRecurrencePattern::ParseBlocks()
	{
		setRoot(m_RecurrencePattern.getBlock());

		terminateBlock();
		auto arpBlock = block{};
		arpBlock.setText(L"Appointment Recurrence Pattern: \r\n");
		arpBlock.addBlock(m_ReaderVersion2, L"ReaderVersion2: 0x%1!08X!\r\n", m_ReaderVersion2.getData());
		arpBlock.addBlock(m_WriterVersion2, L"WriterVersion2: 0x%1!08X!\r\n", m_WriterVersion2.getData());
		arpBlock.addBlock(
			m_StartTimeOffset,
			L"StartTimeOffset: 0x%1!08X! = %1!d! = %2!ws!\r\n",
			m_StartTimeOffset.getData(),
			RTimeToString(m_StartTimeOffset).c_str());
		arpBlock.addBlock(
			m_EndTimeOffset,
			L"EndTimeOffset: 0x%1!08X! = %1!d! = %2!ws!\r\n",
			m_EndTimeOffset.getData(),
			RTimeToString(m_EndTimeOffset).c_str());

		auto exceptions = m_ExceptionCount;
		exceptions.setText(L"ExceptionCount: 0x%1!04X!\r\n", m_ExceptionCount.getData());

		if (!m_ExceptionInfo.empty())
		{
			for (WORD i = 0; i < m_ExceptionInfo.size(); i++)
			{
				auto exception = block{};
				exception.addBlock(
					m_ExceptionInfo[i].StartDateTime,
					L"ExceptionInfo[%1!d!].StartDateTime: 0x%2!08X! = %3!ws!\r\n",
					i,
					m_ExceptionInfo[i].StartDateTime.getData(),
					RTimeToString(m_ExceptionInfo[i].StartDateTime).c_str());
				exception.addBlock(
					m_ExceptionInfo[i].EndDateTime,
					L"ExceptionInfo[%1!d!].EndDateTime: 0x%2!08X! = %3!ws!\r\n",
					i,
					m_ExceptionInfo[i].EndDateTime.getData(),
					RTimeToString(m_ExceptionInfo[i].EndDateTime).c_str());
				exception.addBlock(
					m_ExceptionInfo[i].OriginalStartDate,
					L"ExceptionInfo[%1!d!].OriginalStartDate: 0x%2!08X! = %3!ws!\r\n",
					i,
					m_ExceptionInfo[i].OriginalStartDate.getData(),
					RTimeToString(m_ExceptionInfo[i].OriginalStartDate).c_str());
				auto szOverrideFlags =
					flags::InterpretFlags(flagOverrideFlags, m_ExceptionInfo[i].OverrideFlags);
				exception.addBlock(
					m_ExceptionInfo[i].OverrideFlags,
					L"ExceptionInfo[%1!d!].OverrideFlags: 0x%2!04X! = %3!ws!\r\n",
					i,
					m_ExceptionInfo[i].OverrideFlags.getData(),
					szOverrideFlags.c_str());

				if (m_ExceptionInfo[i].OverrideFlags & ARO_SUBJECT)
				{
					exception.addBlock(
						m_ExceptionInfo[i].SubjectLength,
						L"ExceptionInfo[%1!d!].SubjectLength: 0x%2!04X! = %2!d!\r\n",
						i,
						m_ExceptionInfo[i].SubjectLength.getData());
					exception.addBlock(
						m_ExceptionInfo[i].SubjectLength2,
						L"ExceptionInfo[%1!d!].SubjectLength2: 0x%2!04X! = %2!d!\r\n",
						i,
						m_ExceptionInfo[i].SubjectLength2.getData());

					exception.addBlock(
						m_ExceptionInfo[i].Subject,
						L"ExceptionInfo[%1!d!].Subject: \"%2!hs!\"\r\n",
						i,
						m_ExceptionInfo[i].Subject.c_str());
				}

				if (m_ExceptionInfo[i].OverrideFlags & ARO_MEETINGTYPE)
				{
					auto szFlags = InterpretNumberAsStringNamedProp(
						m_ExceptionInfo[i].MeetingType,
						dispidApptStateFlags,
						const_cast<LPGUID>(&guid::PSETID_Appointment));
					exception.addBlock(
						m_ExceptionInfo[i].MeetingType,
						L"ExceptionInfo[%1!d!].MeetingType: 0x%2!08X! = %3!ws!\r\n",
						i,
						m_ExceptionInfo[i].MeetingType.getData(),
						szFlags.c_str());
				}

				if (m_ExceptionInfo[i].OverrideFlags & ARO_REMINDERDELTA)
				{
					exception.addBlock(
						m_ExceptionInfo[i].ReminderDelta,
						L"ExceptionInfo[%1!d!].ReminderDelta: 0x%2!08X!\r\n",
						i,
						m_ExceptionInfo[i].ReminderDelta.getData());
				}

				if (m_ExceptionInfo[i].OverrideFlags & ARO_REMINDER)
				{
					exception.addBlock(
						m_ExceptionInfo[i].ReminderSet,
						L"ExceptionInfo[%1!d!].ReminderSet: 0x%2!08X!\r\n",
						i,
						m_ExceptionInfo[i].ReminderSet.getData());
				}

				if (m_ExceptionInfo[i].OverrideFlags & ARO_LOCATION)
				{
					exception.addBlock(
						m_ExceptionInfo[i].LocationLength,
						L"ExceptionInfo[%1!d!].LocationLength: 0x%2!04X! = %2!d!\r\n",
						i,
						m_ExceptionInfo[i].LocationLength.getData());
					exception.addBlock(
						m_ExceptionInfo[i].LocationLength2,
						L"ExceptionInfo[%1!d!].LocationLength2: 0x%2!04X! = %2!d!\r\n",
						i,
						m_ExceptionInfo[i].LocationLength2.getData());
					exception.addBlock(
						m_ExceptionInfo[i].Location,
						L"ExceptionInfo[%1!d!].Location: \"%2!hs!\"\r\n",
						i,
						m_ExceptionInfo[i].Location.c_str());
				}

				if (m_ExceptionInfo[i].OverrideFlags & ARO_BUSYSTATUS)
				{
					auto szFlags = InterpretNumberAsStringNamedProp(
						m_ExceptionInfo[i].BusyStatus, dispidBusyStatus, const_cast<LPGUID>(&guid::PSETID_Appointment));
					exception.addBlock(
						m_ExceptionInfo[i].BusyStatus,
						L"ExceptionInfo[%1!d!].BusyStatus: 0x%2!08X! = %3!ws!\r\n",
						i,
						m_ExceptionInfo[i].BusyStatus.getData(),
						szFlags.c_str());
				}

				if (m_ExceptionInfo[i].OverrideFlags & ARO_ATTACHMENT)
				{
					exception.addBlock(
						m_ExceptionInfo[i].Attachment,
						L"ExceptionInfo[%1!d!].Attachment: 0x%2!08X!\r\n",
						i,
						m_ExceptionInfo[i].Attachment.getData());
				}

				if (m_ExceptionInfo[i].OverrideFlags & ARO_SUBTYPE)
				{
					exception.addBlock(
						m_ExceptionInfo[i].SubType,
						L"ExceptionInfo[%1!d!].SubType: 0x%2!08X!\r\n",
						i,
						m_ExceptionInfo[i].SubType.getData());
				}
				if (m_ExceptionInfo[i].OverrideFlags & ARO_APPTCOLOR)
				{
					exception.addBlock(
						m_ExceptionInfo[i].AppointmentColor,
						L"ExceptionInfo[%1!d!].AppointmentColor: 0x%2!08X!\r\n",
						i,
						m_ExceptionInfo[i].AppointmentColor.getData());
				}

				exceptions.addBlock(exception);
			}
		}

		arpBlock.addBlock(exceptions);
		auto reservedBlock1 = m_ReservedBlock1Size;
		reservedBlock1.setText(L"ReservedBlock1Size: 0x%1!08X!", m_ReservedBlock1Size.getData());
		if (m_ReservedBlock1Size)
		{
			reservedBlock1.terminateBlock();
			reservedBlock1.addBlock(m_ReservedBlock1);
		}

		reservedBlock1.terminateBlock();
		arpBlock.addBlock(reservedBlock1);

		if (!m_ExtendedException.empty())
		{
			for (size_t i = 0; i < m_ExtendedException.size(); i++)
			{
				auto exception = block{};
				if (m_WriterVersion2 >= 0x00003009)
				{
					auto szFlags = InterpretNumberAsStringNamedProp(
						m_ExtendedException[i].ChangeHighlight.ChangeHighlightValue,
						dispidChangeHighlight,
						const_cast<LPGUID>(&guid::PSETID_Appointment));
					exception.addBlock(
						m_ExtendedException[i].ChangeHighlight.ChangeHighlightSize,
						L"ExtendedException[%1!d!].ChangeHighlight.ChangeHighlightSize: 0x%2!08X!\r\n",
						i,
						m_ExtendedException[i].ChangeHighlight.ChangeHighlightSize.getData());
					exception.addBlock(
						m_ExtendedException[i].ChangeHighlight.ChangeHighlightValue,
						L"ExtendedException[%1!d!].ChangeHighlight.ChangeHighlightValue: 0x%2!08X! = %3!ws!\r\n",
						i,
						m_ExtendedException[i].ChangeHighlight.ChangeHighlightValue.getData(),
						szFlags.c_str());

					if (m_ExtendedException[i].ChangeHighlight.ChangeHighlightSize > sizeof(DWORD))
					{
						exception.addHeader(L"ExtendedException[%1!d!].ChangeHighlight.Reserved:", i);

						exception.addBlock(m_ExtendedException[i].ChangeHighlight.Reserved);
					}
				}

				exception.addBlock(
					m_ExtendedException[i].ReservedBlockEE1Size,
					L"ExtendedException[%1!d!].ReservedBlockEE1Size: 0x%2!08X!\r\n",
					i,
					m_ExtendedException[i].ReservedBlockEE1Size.getData());
				if (!m_ExtendedException[i].ReservedBlockEE1.empty())
				{
					exception.addBlock(m_ExtendedException[i].ReservedBlockEE1);
				}

				if (i < m_ExceptionInfo.size())
				{
					if (m_ExceptionInfo[i].OverrideFlags & ARO_SUBJECT ||
						m_ExceptionInfo[i].OverrideFlags & ARO_LOCATION)
					{
						exception.addBlock(
							m_ExtendedException[i].StartDateTime,
							L"ExtendedException[%1!d!].StartDateTime: 0x%2!08X! = %3\r\n",
							i,
							m_ExtendedException[i].StartDateTime.getData(),
							RTimeToString(m_ExtendedException[i].StartDateTime).c_str());
						exception.addBlock(
							m_ExtendedException[i].EndDateTime,
							L"ExtendedException[%1!d!].EndDateTime: 0x%2!08X! = %3!ws!\r\n",
							i,
							m_ExtendedException[i].EndDateTime.getData(),
							RTimeToString(m_ExtendedException[i].EndDateTime).c_str());
						exception.addBlock(
							m_ExtendedException[i].OriginalStartDate,
							L"ExtendedException[%1!d!].OriginalStartDate: 0x%2!08X! = %3!ws!\r\n",
							i,
							m_ExtendedException[i].OriginalStartDate.getData(),
							RTimeToString(m_ExtendedException[i].OriginalStartDate).c_str());
					}

					if (m_ExceptionInfo[i].OverrideFlags & ARO_SUBJECT)
					{
						exception.addBlock(
							m_ExtendedException[i].WideCharSubjectLength,
							L"ExtendedException[%1!d!].WideCharSubjectLength: 0x%2!08X! = %2!d!\r\n",
							i,
							m_ExtendedException[i].WideCharSubjectLength.getData());
						exception.addBlock(
							m_ExtendedException[i].WideCharSubject,
							L"ExtendedException[%1!d!].WideCharSubject: \"%2!ws!\"\r\n",
							i,
							m_ExtendedException[i].WideCharSubject.c_str());
					}

					if (m_ExceptionInfo[i].OverrideFlags & ARO_LOCATION)
					{
						exception.addBlock(
							m_ExtendedException[i].WideCharLocationLength,
							L"ExtendedException[%1!d!].WideCharLocationLength: 0x%2!08X! = %2!d!\r\n",
							i,
							m_ExtendedException[i].WideCharLocationLength.getData());
						exception.addBlock(
							m_ExtendedException[i].WideCharLocation,
							L"ExtendedException[%1!d!].WideCharLocation: \"%2!ws!\"\r\n",
							i,
							m_ExtendedException[i].WideCharLocation.c_str());
					}
				}

				exception.addBlock(
					m_ExtendedException[i].ReservedBlockEE2Size,
					L"ExtendedException[%1!d!].ReservedBlockEE2Size: 0x%2!08X!\r\n",
					i,
					m_ExtendedException[i].ReservedBlockEE2Size.getData());
				if (m_ExtendedException[i].ReservedBlockEE2Size)
				{
					exception.addBlock(m_ExtendedException[i].ReservedBlockEE2);
				}

				arpBlock.addBlock(exception);
			}
		}

		auto reservedBlock2 = m_ReservedBlock2Size;
		reservedBlock2.setText(L"ReservedBlock2Size: 0x%1!08X!", m_ReservedBlock2Size.getData());
		if (m_ReservedBlock2Size)
		{
			reservedBlock2.terminateBlock();
			reservedBlock2.addBlock(m_ReservedBlock2);
		}

		reservedBlock2.terminateBlock();
		arpBlock.addBlock(reservedBlock2);
		addBlock(arpBlock);
	}
} // namespace smartview