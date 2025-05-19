"use client";

import * as React from "react";
import {
  format,
  setHours,
  setMinutes,
  setSeconds,
  isValid,
  getHours,
} from "date-fns";
import { Calendar as CalendarIcon, Clock } from "lucide-react";

import { cn } from "@/lib/utils";
import { Button } from "@/components/ui/button";
import { Calendar } from "@/components/ui/calendar";
import { Input } from "@/components/ui/input";
import {
  Popover,
  PopoverContent,
  PopoverTrigger,
} from "@/components/ui/popover";
import { Label } from "./label";
import { ToggleGroup, ToggleGroupItem } from "@/components/ui/toggle-group"; // Import ToggleGroup

interface DateTimePickerProps {
  date: Date | undefined;
  setDate: (date: Date | undefined) => void;
  disabled?: boolean;
}

export function DateTimePicker({ date, setDate, disabled }: DateTimePickerProps) {
  const [calendarOpen, setCalendarOpen] = React.useState(false);

  // Get initial hour in 12-hour format
  const initialHour12 = date && isValid(date) ? parseInt(format(date, "hh"), 10) : null;
  const initialAmPm = date && isValid(date) ? (format(date, "a") as "AM" | "PM") : "AM";

  // State for time components using 12-hour format
  const [hourInput, setHourInput] = React.useState<string>(
    initialHour12 !== null ? initialHour12.toString() : ""
  );
  const [minutes, setMinutesState] = React.useState<string>(
    date && isValid(date) ? format(date, "mm") : ""
  );
  const [amPm, setAmPm] = React.useState<"AM" | "PM">(initialAmPm);

  React.useEffect(() => {
    // Update time inputs if the date prop changes externally
    if (date && isValid(date)) {
      const currentHour24 = getHours(date);
      const currentHour12 = currentHour24 % 12 === 0 ? 12 : currentHour24 % 12;
      setHourInput(currentHour12.toString());
      setMinutesState(format(date, "mm"));
      setAmPm(currentHour24 < 12 ? "AM" : "PM");
    } else {
      setHourInput("");
      setMinutesState("");
      setAmPm("AM"); // Default to AM if date is cleared
    }
  }, [date]);

  const handleDateSelect = (selectedDate: Date | undefined) => {
    if (!selectedDate) {
      setDate(undefined);
      return;
    }
    updateDateFromInputs(selectedDate, hourInput, minutes, amPm);
    setCalendarOpen(false);
  };

  // Update Date object based on all inputs
  const updateDateFromInputs = (
    baseDateInput: Date | undefined,
    hrInput: string,
    minInput: string,
    ampmInput: "AM" | "PM"
  ) => {
    const numHours12 = parseInt(hrInput, 10);
    const numMinutes = parseInt(minInput, 10);

    if (
      !isNaN(numHours12) &&
      numHours12 >= 1 &&
      numHours12 <= 12 &&
      !isNaN(numMinutes) &&
      numMinutes >= 0 &&
      numMinutes <= 59
    ) {
      let numHours24 = numHours12;
      if (ampmInput === "PM" && numHours12 !== 12) {
        numHours24 += 12;
      } else if (ampmInput === "AM" && numHours12 === 12) {
        // Midnight case
        numHours24 = 0;
      }

      const baseDate = baseDateInput && isValid(baseDateInput) ? new Date(baseDateInput) : new Date();
      let newDate = setHours(baseDate, numHours24);
      newDate = setMinutes(newDate, numMinutes);
      newDate = setSeconds(newDate, 0); // Reset seconds

      if (isValid(newDate)) {
        // Only update if the resulting date is valid and different
        if (!date || newDate.getTime() !== date.getTime()) {
          setDate(newDate);
        }
      }
    }
  };

  // Generic handler for time input changes (Hours and Minutes)
  const handleTimeInputChange = (
    e: React.ChangeEvent<HTMLInputElement>,
    setter: React.Dispatch<React.SetStateAction<string>>,
    max: number,
    min: number = 0 // Add min for hours (1-12)
  ) => {
    let value = e.target.value;
    value = value.replace(/\D/g, ""); // Remove non-digits

    let currentHourStr = hourInput;
    let currentMinStr = minutes;
    const currentAmPm = amPm;

    if (value !== "") {
      const numValue = parseInt(value, 10);
      if (numValue > max) value = max.toString();
      if (numValue < min && value.length > 0) value = min.toString(); // Enforce min for hours

      // Update the specific state
      if (setter === setHourInput) {
         currentHourStr = value;
      } else if (setter === setMinutesState) {
         currentMinStr = value;
      }
       setter(value);
    } else {
       // Handle empty input
       if (setter === setHourInput) currentHourStr = "";
       if (setter === setMinutesState) currentMinStr = "";
       setter("");
    }

    // Attempt to update the date immediately if possible
    updateDateFromInputs(date, currentHourStr, currentMinStr, currentAmPm);
  };

  const handleAmPmChange = (value: "AM" | "PM") => {
    if (value) { // Ensure value is either 'AM' or 'PM'
        setAmPm(value);
        updateDateFromInputs(date, hourInput, minutes, value);
    }
  }

  return (
    <Popover open={calendarOpen} onOpenChange={setCalendarOpen}>
      <PopoverTrigger asChild disabled={disabled}>
        <Button
          variant={"outline"}
          className={cn(
            "w-full justify-start text-left font-normal",
            !date && "text-muted-foreground"
          )}
        >
          <CalendarIcon className="mr-2 h-4 w-4" />
          {date && isValid(date) ? (
            format(date, "PPP hh:mm a") // Use 12-hour format with AM/PM
          ) : (
            <span>Pick a date and time</span>
          )}
        </Button>
      </PopoverTrigger>
      <PopoverContent className="w-auto p-0">
        <Calendar
          mode="single"
          selected={date}
          onSelect={handleDateSelect}
          initialFocus
          disabled={disabled}
        />
        <div className="p-3 border-t border-border">
          <Label className="flex items-center text-sm font-medium text-muted-foreground mb-2">
            <Clock className="mr-1 h-4 w-4" /> Time
          </Label>
          <div className="flex items-center space-x-2">
            <Input
              type="number"
              min="1" // Hour range 1-12
              max="12"
              value={hourInput}
              onChange={(e) => handleTimeInputChange(e, setHourInput, 12, 1)}
              placeholder="hh"
              className="w-14 [appearance:textfield] [&::-webkit-outer-spin-button]:appearance-none [&::-webkit-inner-spin-button]:appearance-none"
              aria-label="Hours (1-12)"
              disabled={disabled}
            />
            <span>:</span>
            <Input
              type="number"
              min="0"
              max="59"
              value={minutes}
              onChange={(e) => handleTimeInputChange(e, setMinutesState, 59)}
              onBlur={() => setMinutesState(prev => prev.length === 1 ? '0' + prev : prev)} // Pad minutes on blur
              placeholder="mm"
              className="w-14 [appearance:textfield] [&::-webkit-outer-spin-button]:appearance-none [&::-webkit-inner-spin-button]:appearance-none"
              aria-label="Minutes"
              disabled={disabled}
            />
            <ToggleGroup
                type="single"
                value={amPm}
                onValueChange={handleAmPmChange}
                aria-label="Select AM or PM"
                disabled={disabled}
                className="ml-auto"
            >
                <ToggleGroupItem value="AM" aria-label="AM" className="px-2 h-9 text-xs">
                    AM
                </ToggleGroupItem>
                <ToggleGroupItem value="PM" aria-label="PM" className="px-2 h-9 text-xs">
                    PM
                </ToggleGroupItem>
            </ToggleGroup>
          </div>
        </div>
      </PopoverContent>
    </Popover>
  );
}