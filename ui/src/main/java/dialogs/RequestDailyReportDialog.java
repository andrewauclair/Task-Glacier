package dialogs;

import packets.RequestDailyReport;
import packets.RequestID;
import raven.datetime.DatePicker;
import taskglacier.MainFrame;
import util.DialogEscape;

import javax.swing.*;
import java.awt.*;
import java.time.LocalDate;

public class RequestDailyReportDialog extends JDialog {
    public RequestDailyReportDialog(MainFrame mainFrame) {
        setTitle("Request Daily Report");
        
        setLayout(new GridBagLayout());
        setModal(true);

        DialogEscape.addEscapeHandler(this);

        DatePicker picker = new DatePicker();
        picker.setSelectedDate(LocalDate.now());

        GridBagConstraints gbc = new GridBagConstraints();
        gbc.anchor = GridBagConstraints.CENTER;
        gbc.insets = new Insets(5, 5, 5, 5);
        gbc.gridx = 0;
        gbc.gridy = 0;
        gbc.weightx = 0;
        gbc.weighty = 0;

        add(picker, gbc);
        gbc.gridy++;

        JButton send = new JButton("Send");

        add(send, gbc);
        gbc.gridy++;

        pack();

        // center on the main frame
        setLocationRelativeTo(mainFrame);

        send.addActionListener(e -> {
            LocalDate localDate = picker.getSelectedDate();

            int year = localDate.getYear();
            int month = localDate.getMonthValue();
            int day = localDate.getDayOfMonth();

            RequestDailyReport request = new RequestDailyReport();
            request.requestID = RequestID.nextRequestID();
            request.month = month;
            request.day = day;
            request.year = year;

            mainFrame.getConnection().sendPacket(request);

            RequestDailyReportDialog.this.dispose();
        });
    }
}
