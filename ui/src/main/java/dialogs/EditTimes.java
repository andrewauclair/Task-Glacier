package dialogs;

import data.Task;
import data.TimeData;
import packets.RequestID;
import packets.UpdateTask;
import taskglacier.MainFrame;

import javax.swing.*;
import java.awt.*;
import java.util.Optional;

/*
Display a selection for the time category and code to use for this task

In addition to that, provide advanced options for modifying the time category and code for past start/stops
 */
public class EditTimes extends JDialog {
    private final TimeData timeData;

    public EditTimes(MainFrame mainFrame, Task task) {
        timeData = mainFrame.getTimeData();

        JComboBox<String> timeCategory = new JComboBox<>();
        JComboBox<String> timeCode = new JComboBox<>();

        for (TimeData.TimeCategory category : timeData.getTimeCategories()) {
            timeCategory.addItem(category.name);
        }

        setLayout(new FlowLayout());

        add(new JLabel("Time Category"));
        add(timeCategory);
        add(new JLabel("Time Code"));
        add(timeCode);

        timeCategory.addActionListener(e -> {
            Optional<TimeData.TimeCategory> timeCategory2 = timeData.getTimeCategories().stream()
                    .filter(timeCategory1 -> timeCategory1.name.equals(timeCategory.getSelectedItem()))
                    .findFirst();

            if (timeCategory2.isPresent()) {
                timeCode.removeAllItems();
                for (TimeData.TimeCode code : timeCategory2.get().timeCodes) {
                    timeCode.addItem(code.name);
                }
            }
        });

        timeCode.addActionListener(e -> {

        });

        JButton save = new JButton("Save");

        save.addActionListener(e -> {
            UpdateTask update = new UpdateTask(RequestID.nextRequestID(), task);
            update.timeCodes.clear();



            mainFrame.getConnection().sendPacket(update);

            EditTimes.this.dispose();
        });

        add(save);
    }
}
